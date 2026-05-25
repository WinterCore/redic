#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#include "potty.h"

size_t get_mutation_size(SepticTankMutation *mutation) {
    switch (mutation->type) {
        case SEPTIC_TANK_GET:
        case SEPTIC_TANK_TTL:
            UNREACHABLE();
            break;

        case SEPTIC_TANK_DEL:
            return (
                1                                   + // Type
                sizeof(mutation->set.key->len)      + // Key Length
                mutation->set.key->len                // Key
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
    }
}

void serialize_mutation(SepticTankMutation *mutation, FILE *aof) {
    uint64_t record_length = get_mutation_size(mutation);

    UNIMPLEMENTED("Serialize mutation");
}

void perform_flush(FlushJob *job) {
    FILE *aof = fopen("data.aof", "a");
    
    for (size_t i = 0, n = hector_size(job->waste); i < n; i += 1) {
        SepticTankMutation *mutation = *(SepticTankMutation **) hector_get(job->waste, i);
        UNIMPLEMENTED("Flush mutation to disk");
    }
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


        // TODO: Flush all data to disk

        DEBUG_PRINTF("Flushing to disk ts=%llu\n", job->ts);
        perform_flush(job);
        
        arena_destroy(job->arena);
    }

    return NULL;
}
