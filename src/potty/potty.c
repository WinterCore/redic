#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>

#include "potty.h"
#include "potty_flusher.h"

#define FLUSH_TIMEOUT 1000
#define FLUSH_ITEMS_THRESHOLD 500

Potty *potty_create(Sewer *sewer) {
    Potty *potty = malloc(sizeof(Potty));
    potty->sewer = sewer;
    potty->arena = arena_create();
    // Waste is allocated with arena because it needs to be freed once the data is flushed
    potty->waste = hector_create(potty->arena, sizeof(SepticTankMutation *), 500);

    // Here we don't allocate with arena because this hector will be used for the entirety of the program
    potty->flush_jobs = hector_create(NULL, sizeof(FlushJob *), 30);
    pthread_mutex_init(&potty->flusher_mutex, NULL);
    potty->flusher_tid = 0;
    potty->flusher_running = false;

    potty->last_flush_ts = monotonic_now_ms();

    return potty;
}


void potty_destroy(Potty *potty) {
    UNIMPLEMENTED("Should do graceful shutdown");
    // TODO: Need to handle graceful shutdown which is a whole other can of worms
    /*
    arena_destroy(potty->arena);
    
    FlushJob *job = NULL;
    for (size_t i = 0; (job = hector_get(potty->flush_jobs, i)); i += 1) {
        arena_destroy(job->arena);
    }

    if (potty->flusher_running) {

    }

    hector_destroy(potty->flush_jobs);
    pthread_mutex_destroy(&potty->flusher_mutex);
    free(potty);
    */
}


bool should_flush(Potty *potty) {
    size_t log_size = hector_size(potty->waste);

    if (log_size >= FLUSH_ITEMS_THRESHOLD) {
        return true;
    }

    uint64_t now_ms = monotonic_now_ms();

    if (potty->last_flush_ts + FLUSH_TIMEOUT < now_ms) {
        return true;
    }

    return false;
}

void potty_flush(Potty *potty) {
    uint64_t now = monotonic_now_ms();
    if (hector_size(potty->waste) == 0) {
        potty->last_flush_ts = now;
        return;
    }

    FlushJob *job = arena_alloc(potty->arena, sizeof(FlushJob));
    job->arena = potty->arena;
    job->waste = potty->waste;
    job->ts = now;

    potty->arena = arena_create();
    potty->waste = hector_create(potty->arena, sizeof(SepticTankMutation *), 500);

    pthread_mutex_lock(&potty->flusher_mutex);
    hector_push(potty->flush_jobs, &job);

    if (!potty->flusher_running) {
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        pthread_create(&tid, &attr, potty_drain, potty);
        pthread_attr_destroy(&attr);

        potty->flusher_running = true;
        potty->flusher_tid = tid;
    }

    potty->last_flush_ts = now;
    pthread_mutex_unlock(&potty->flusher_mutex);
}

void potty_poop(Potty *potty, SepticTankMutation *mutation) {
    SepticTankMutation *cloned_mutation = septic_tank_mutation_clone(potty->arena, mutation);

    hector_push(potty->waste, &cloned_mutation);
}

void *potty_pump(void *input) {
    Potty *potty = input;
    
    // Poop indefinitely...
    while (1) {
        SewerMessage *message = sewer_timed_consume(potty->sewer, 1000);

        if (message != NULL) {
            potty_poop(potty, message->value);
            sewer_message_destroy(message, true);
        }

        if (should_flush(potty)) {
            potty_flush(potty);
        }
    }
}

pthread_t potty_launch(Potty *potty) {
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&tid, &attr, potty_pump, potty);
    pthread_attr_destroy(&attr);

    return tid;
}


void potty_feed(Potty *potty, SepticTankMutation *mutation) {
    Arena *message_arena = arena_create();
    SepticTankMutation *cloned_mutation = septic_tank_mutation_clone(message_arena, mutation);

    SewerMessage *message = sewer_message_create(message_arena, cloned_mutation, false);

    // Send
    sewer_send(potty->sewer, message);
}
