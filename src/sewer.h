#ifndef SEWER_H
#define SEWER_H

#include <pthread.h>

#include "arena.h"
#include "ring.h"

typedef struct Sewer {
    RingBuf *buffer;
    pthread_mutex_t mutex;

    pthread_cond_t has_space;
    pthread_cond_t has_items;
} Sewer;

typedef struct SewerMessage {
    void *value;

    // Response sewer, single use ONLY
    Sewer *clogged_sewer;
    bool is_consumed;

    // Arena used to allocate the message and its contents
    Arena *arena;
} SewerMessage;

Sewer *sewer_create(size_t cap);
void sewer_destroy(Sewer *sewer);

void sewer_send(Sewer *sewer, SewerMessage *message);
SewerMessage *sewer_consume(Sewer *sewer);

SewerMessage *sewer_message_create(Arena *arena, void *value, bool with_response);
void sewer_message_destroy(SewerMessage *message, bool destroy_clogged_sewer);

#endif
