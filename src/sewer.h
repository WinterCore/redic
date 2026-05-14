#ifndef SEWER_H
#define SEWER_H

#include <pthread.h>

#include "arena.h"
#include "ring.h"

/**
 * Bounded thread-safe queue for passing `SewerMessage*` between actors.
 */
typedef struct Sewer {
    RingBuf *buffer;
    pthread_mutex_t mutex;

    pthread_cond_t has_space;
    pthread_cond_t has_items;
} Sewer;

/**
 * Queue payload wrapper.
 * `value` points to caller-defined data allocated from `arena`.
 * If `clogged_sewer` is set, it is a single-use response channel.
 */
typedef struct SewerMessage {
    void *value;

    // Response sewer, single use ONLY
    Sewer *clogged_sewer;
    bool is_consumed;

    // Arena used to allocate the message and its contents
    Arena *arena;
} SewerMessage;

/**
 * Creates a sewer with capacity `cap` messages.
 */
Sewer *sewer_create(size_t cap);

/**
 * Destroys sewer synchronization primitives and queue storage.
 */
void sewer_destroy(Sewer *sewer);

/**
 * Enqueues a message, blocking while the queue is full.
 */
void sewer_send(Sewer *sewer, SewerMessage *message);

/**
 * Dequeues one message, blocking while the queue is empty.
 */
SewerMessage *sewer_consume(Sewer *sewer);

/**
 * Dequeues one message, blocking until a message is available or the timeout 
 * (in milliseconds) expires. Returns NULL if the timeout expires.
 */
SewerMessage *sewer_timed_consume(Sewer *sewer, int timeout_ms);

/**
 * Allocates a message in `arena`.
 * When `with_response` is true, also creates a single-item response sewer.
 */
SewerMessage *sewer_message_create(Arena *arena, void *value, bool with_response);

/**
 * Destroys message arena and optionally its response sewer.
 */
void sewer_message_destroy(SewerMessage *message, bool destroy_clogged_sewer);

#endif
