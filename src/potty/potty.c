#include "potty.h"
#include <pthread.h>
#include <time.h>
#include <stdlib.h>


Potty *potty_create(Sewer *sewer) {
    Potty *potty = malloc(sizeof(Potty));
    potty->sewer = sewer;
    potty->arena = arena_create();
    // Waste is allocated with arena because it needs to be freed once the data is flushed
    potty->waste = hector_create(potty->arena, sizeof(SepticTankMutation), 500);

    // Here we don't allocate with arena because this hector will be used for the entirety of the program
    potty->flush_jobs = hector_create(NULL, sizeof(FlushJob), 20);
    pthread_mutex_init(&potty->flusher_mutex, NULL);
    potty->flusher_pid = 0;
    potty->flusher_running = false;

    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    potty->last_flush_ts = t.tv_sec * 1000L + t.tv_nsec / 1000000;

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

void *potty_pump(void *input) {
    Potty *potty = input;
    SewerMessage *message = malloc(sizeof(SewerMessage));
    
    // Poop indefinitely...
    while (1) {
        sewer_consume(potty->sewer, message);
        potty_poop(potty, message->value);
    }

    free(message);
}

pthread_t potty_launch(Sewer *sewer) {
    Potty *potty = potty_create(sewer);

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&tid, &attr, potty_pump, potty);
    pthread_attr_destroy(&attr);

    return tid;
}


void potty_poop(Potty *potty, SepticTankMutation *mutation) {

}
