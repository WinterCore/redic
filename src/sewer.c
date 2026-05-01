#include "sewer.h"
#include "ring.h"
#include <pthread.h>
#include <stdlib.h>

Sewer *sewer_create(size_t cap) {
    Sewer *sewer = malloc(sizeof(Sewer));

    pthread_mutex_init(&sewer->mutex, NULL);
    pthread_cond_init(&sewer->has_items, NULL);
    pthread_cond_init(&sewer->has_space, NULL);
    sewer->buffer = ringbuf_create(cap, sizeof(SewerMessage));

    return sewer;
}

void sewer_destroy(Sewer *sewer) {
    ringbuf_destroy(sewer->buffer);
    free(sewer);
}

void sewer_send(Sewer *sewer, SewerMessage *message) {
    pthread_mutex_lock(&sewer->mutex); 
    
    // Block if the buffer is full
    while (ringbuf_is_full(sewer->buffer)) {
        pthread_cond_wait(&sewer->has_space, &sewer->mutex);
    }
    
    ringbuf_push(sewer->buffer, message);
    pthread_cond_signal(&sewer->has_items);

    pthread_mutex_unlock(&sewer->mutex); 
}

void sewer_consume(Sewer *sewer, SewerMessage *out_message) {
    pthread_mutex_lock(&sewer->mutex);

    while (ringbuf_is_empty(sewer->buffer)) {
        pthread_cond_wait(&sewer->has_items, &sewer->mutex);
    }

    ringbuf_pop(sewer->buffer, out_message);
    pthread_cond_signal(&sewer->has_space);

    pthread_mutex_unlock(&sewer->mutex);
}

SewerMessage *sewer_message_create(void *value, bool with_response) {
    SewerMessage *message = malloc(sizeof(SewerMessage));
    
    message->is_consumed = false;
    message->blocked_sewer = with_response
        ? sewer_create(1)
        : NULL;
    message->value = value;

    return message;
}

void sewer_message_destroy(SewerMessage *message) {
    if (message->blocked_sewer != NULL) {
        sewer_destroy(message->blocked_sewer);
    }
    free(message);
}
