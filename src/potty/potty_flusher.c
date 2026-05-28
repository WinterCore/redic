#define _DARWIN_C_SOURCE 1
#define _GNU_SOURCE 1
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "potty.h"

// @deadcode for now, not sure if it'll be needed later
size_t get_mutation_size(SepticTankMutation *mutation) {
    switch (mutation->type) {
        case SEPTIC_TANK_GET:
        case SEPTIC_TANK_TTL:
        case SEPTIC_TANK_EXISTS:
        case SEPTIC_TANK_INCRBY:
            UNREACHABLE();
            break;

        case SEPTIC_TANK_DEL:
            return (
                1                                   + // Type
                sizeof(mutation->del.key->len)      + // Key Length
                mutation->del.key->len                // Key
            );

        case SEPTIC_TANK_SET:
            return (
                1                                   + // Type
                sizeof(mutation->set.expiration.ts) + // Expiration

                sizeof(mutation->set.key->len)      + // Key Length
                mutation->set.key->len              + // Key

                sizeof(mutation->set.value->len)    + // Key Length
                mutation->set.value->len              // Value
            );
            break;

        case SEPTIC_TANK_EXPIRE:
            return (
                1                                     + // Type
                sizeof(mutation->expire.key->len)     + // Key Length
                mutation->expire.key->len             + // Key
                sizeof(mutation->expire.expires_at)     // Expiration
            );
            break;
    }
}

void fwrite_string(FILE *aof, DataString *str) {
    uint64_t len = str->len;
    uint64_t items = fwrite(&len, sizeof(len), 1, aof);

    if (items != 1) {
        PANIC("fwrite_string failed");
    }

    items = fwrite(str->str, len, 1, aof);

    if (items != 1) {
        PANIC("fwrite_string failed");
    }
}

void fwrite_int64(FILE *aof, int64_t *value) {
    uint64_t items = fwrite(value, sizeof(int64_t), 1, aof);
    
    if (items != 1) {
        PANIC("fwrite_int64 failed");
    }
}

void fwrite_byte(FILE *aof, uint8_t *value) {
    uint64_t items = fwrite(value, sizeof(uint8_t), 1, aof);
    
    if (items != 1) {
        PANIC("fwrite_byte failed");
    }
}

void serialize_mutation_to_disk(FILE *aof, SepticTankMutation *mutation) {
    uint8_t type = mutation->type;
    fwrite_byte(aof, &type); // Type

    switch (mutation->type) {
        case SEPTIC_TANK_SET: {
            // Expiration
            fwrite_int64(aof, &mutation->set.expiration.ts);

            // Key
            fwrite_string(aof, mutation->set.key);

            // Value
            fwrite_string(aof, mutation->set.value);

            break;
        }
        case SEPTIC_TANK_DEL: {
            // Key
            fwrite_string(aof, mutation->del.key);

            break;
        }
        case SEPTIC_TANK_EXPIRE: {
            // Key
            fwrite_string(aof, mutation->expire.key);

            // Expiration
            fwrite_int64(aof, &mutation->expire.expires_at);

            break;
        }
        case SEPTIC_TANK_GET:
        case SEPTIC_TANK_TTL:
        case SEPTIC_TANK_EXISTS:
        case SEPTIC_TANK_INCRBY:
            UNREACHABLE();
    }
}

void perform_flush(FlushJob *job) {
    FILE *aof = fopen("data.aof", "a");
    
    if (aof == NULL) {
        PANIC("Failed to open AOF!");
    }
    
    for (size_t i = 0, n = hector_size(job->waste); i < n; i += 1) {
        SepticTankMutation *mutation = *(SepticTankMutation **) hector_get(job->waste, i);

        serialize_mutation_to_disk(aof, mutation);
    }

    int fd = fileno(aof);

    // TODO: Should probably follow redis and have a deterministic flush timing but this is fine for now
    #ifdef __APPLE__
        // macOS/iOS specific absolute durability flush
        fcntl(fd, F_FULLFSYNC, 0);
    #else
        // Standard POSIX flush for Linux and other Unix-like systems
        fsync(fd);
    #endif
    fclose(aof);
}


void *potty_drain(void *input) {
    Potty *potty = input;

    while (1) {
        // Try to grab a flush job
        pthread_mutex_lock(&potty->flusher_mutex);

        if (hector_size(potty->flush_jobs) == 0) {
            potty->flusher_running = false;
            pthread_mutex_unlock(&potty->flusher_mutex);
            break;
        }

        FlushJob *job = *(FlushJob **) hector_get(potty->flush_jobs, 0);
        hector_splice(potty->flush_jobs, 0, 1);
        pthread_mutex_unlock(&potty->flusher_mutex);

        perform_flush(job);
        
        arena_destroy(job->arena);
    }

    return NULL;
}
