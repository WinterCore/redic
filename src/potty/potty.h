#ifndef POTTY_H
#define POTTY_H

#include <inttypes.h>
#include <pthread.h>

#include "../aids.h"
#include "../sewer.h"
#include "../septic_tank/septic_tank_operation.h"

/**
 * Append-only persistence layer for write operations. Replay on startup reconstructs the in-memory state.

  What gets logged

  SepticTankOperation (mutations only — SET, DEL). Pushed from inside the septic tank actor itself, since:
  - It already knows whether a write succeeded (NX/XX/WRONGTYPE)
  - Single-threaded ordering is automatic
  - Operations are the canonical representation; no need to drag RESP through

  Potty struct

  - Hector *waste + Arena *arena — the current buffer being filled
  - Hector *flush_jobs — queue of FlushJob { Hector *waste; Arena *arena } waiting to be flushed
  - pthread_t flusher_pid + bool flusher_running + pthread_mutex_t flusher_mutex — single shared mutex protects
   both the flusher state and the queue
  - uint64_t last_flush_ts — monotonic timestamp from clock_gettime(CLOCK_MONOTONIC) to drive the time-based
  flush
  - Size + time thresholds as hardcoded macros for now

  Flush triggers

  1. Size: waste length hits threshold
  2. Time: idle period exceeds threshold (potty actor checks during downtime between push commands)

  Flush flow (on threshold hit)

  1. Lock flusher_mutex
  2. Push current {waste, arena} onto flush_jobs
  3. Replace waste and arena with fresh ones (so writes keep flowing)
  4. If flusher_running == false, set it true and pthread_create the flusher
  5. Unlock

  Flusher thread

  - Iterates flush_jobs one job at a time
  - For each job: write to flush_<ts>.tmp → fsync → close → rename to flush_<ts>.aof
  - Frees the hector + arena after success
  - Before exiting: lock mutex, double-check queue is empty, set flusher_running = false, unlock, exit. The
  double-check closes the race where potty queues a job while the flusher is winding down.

  Crash safety

  - Partial writes live in .tmp files; startup ignores them
  - Up to one flush interval of writes can be lost on crash (Redis-style everysec tradeoff)
  - No length prefixes / checksums for now — accepted simplification

  Replay (on startup)

  - Scan directory for flush_*.aof files
  - Sort by timestamp in filename
  - Feed each operation directly into the septic tank in order
 *
 */


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
    pthread_t flusher_pid;
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
