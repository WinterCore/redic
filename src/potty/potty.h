#ifndef POTTY_H
#define POTTY_H

#include <inttypes.h>
#include <pthread.h>

#include "../aids.h"
#include "../sewer.h"
#include "../septic_tank/septic_tank_operation.h"

typedef struct FlushJob {
    // Serialized mutation buffer waiting to be flushed.
    Hector *waste;
    // Arena that owns `waste` and its entries.
    Arena *arena;
    
    // Monotonic timestamp captured when this flush job was queued.
    uint64_t ts;
} FlushJob;

typedef struct Potty {
    // Inbound mutation queue consumed by the potty actor thread.
    Sewer *sewer;

    // Active in-memory append buffer for mutations not yet flushed to disk.
    Hector *waste;
    // Arena that owns `waste` and active mutation entries.
    Arena *arena;

    // Detached thread id for the current flusher worker (if running).
    pthread_t flusher_tid;
    // True while a flusher thread is active.
    bool flusher_running;
    // Guards `flusher_running` and `flush_jobs`.
    pthread_mutex_t flusher_mutex;
    // Queue of pending flush batches to write to disk.
    Hector *flush_jobs;

    // Last monotonic time a flush occurred or was scheduled (ms).
    uint64_t last_flush_ts;
} Potty;

Potty *potty_create(Sewer *sewer);
void potty_destroy(Potty *potty);

pthread_t potty_launch(Potty *potty);

void potty_feed(Potty *potty, SepticTankMutation *mutation);

#endif
