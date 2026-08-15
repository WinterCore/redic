#define _DARWIN_C_SOURCE 1
#define _GNU_SOURCE 1
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "potty.h"
#include "potty_serializer.h"

void perform_flush(FlushJob *job) {
    DEBUG_PRINTF("AOF: Flushing data to disk...\n\tItem count: %zu\n", hector_size(job->waste));
    FILE *aof = fopen("data.aof", "a");
    
    if (aof == NULL) {
        PANIC("Failed to open AOF!");
    }
    
    for (size_t i = 0, n = hector_size(job->waste); i < n; i += 1) {
        SepticTankMutation *mutation = *(SepticTankMutation **) hector_get(job->waste, i);

        potty_serialize_mutation(aof, mutation);
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
