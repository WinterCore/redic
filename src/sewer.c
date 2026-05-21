#include "sewer.h"
#include "aids.h"
#include "arena.h"
#include "ring.h"
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

Sewer *sewer_create(size_t cap) {
    Sewer *sewer = malloc(sizeof(Sewer));

    pthread_mutex_init(&sewer->mutex, NULL);

    pthread_cond_init(&sewer->has_items, NULL);
    pthread_cond_init(&sewer->has_space, NULL);

    sewer->buffer = ringbuf_create(cap, sizeof(SewerMessage *));

    return sewer;
}

void sewer_destroy(Sewer *sewer) {
    ringbuf_destroy(sewer->buffer);
    pthread_mutex_destroy(&sewer->mutex);
    pthread_cond_destroy(&sewer->has_items);
    pthread_cond_destroy(&sewer->has_space);
    free(sewer);
}

void sewer_send(Sewer *sewer, SewerMessage *message) {
    pthread_mutex_lock(&sewer->mutex); 
    
    // Block if the buffer is full
    while (ringbuf_is_full(sewer->buffer)) {
        pthread_cond_wait(&sewer->has_space, &sewer->mutex);
    }
    
    ringbuf_push(sewer->buffer, &message);
    pthread_cond_signal(&sewer->has_items);

    pthread_mutex_unlock(&sewer->mutex); 
}

SewerMessage *sewer_consume(Sewer *sewer) {
    pthread_mutex_lock(&sewer->mutex);

    while (ringbuf_is_empty(sewer->buffer)) {
        pthread_cond_wait(&sewer->has_items, &sewer->mutex);
    }

    SewerMessage *out_message = NULL;

    ringbuf_pop(sewer->buffer, &out_message);
    pthread_cond_signal(&sewer->has_space);

    pthread_mutex_unlock(&sewer->mutex);

    return out_message;
}

SewerMessage *sewer_timed_consume(Sewer *sewer, int timeout_ms) {
    pthread_mutex_lock(&sewer->mutex);

    struct timespec timeout_ts;
    clock_gettime(CLOCK_REALTIME, &timeout_ts);
    timeout_ts.tv_sec += timeout_ms / 1000;
    timeout_ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    
    if (timeout_ts.tv_nsec >= 1000000000) {
        timeout_ts.tv_sec += 1;
        timeout_ts.tv_nsec -= 1000000000;
    }

    while (ringbuf_is_empty(sewer->buffer)) {
        int result = pthread_cond_timedwait(&sewer->has_items, &sewer->mutex, &timeout_ts);

        if (result == ETIMEDOUT) {
            pthread_mutex_unlock(&sewer->mutex);
            return NULL;
        }
    }

    SewerMessage *out_message = NULL;

    ringbuf_pop(sewer->buffer, &out_message);
    pthread_cond_signal(&sewer->has_space);

    pthread_mutex_unlock(&sewer->mutex);

    return out_message;
}

SewerMessage *sewer_message_create(Arena *arena, void *value, bool with_response) {
    if (arena == NULL) {
        PANIC("Arena must not be null");
    }

    SewerMessage *message = arena_alloc(arena, sizeof(SewerMessage));
    
    message->arena = arena;
    message->is_consumed = false;
    message->clogged_sewer = with_response
        ? sewer_create(1)
        : NULL;
    message->value = value;

    return message;
}

void sewer_message_destroy(SewerMessage *message, bool destroy_clogged_sewer) {
    if (destroy_clogged_sewer && message->clogged_sewer != NULL) {
        sewer_destroy(message->clogged_sewer);
    }

    arena_destroy(message->arena);
}
