#include <pthread.h>

#include "potty.h"

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

        
        arena_destroy(job->arena);
    }

    return NULL;
}
