#ifndef SEWER_H
#define SEWER_H

#include <pthread.h>

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
    Sewer *blocked_sewer;
    bool is_consumed;
} SewerMessage;

Sewer *sewer_create(size_t cap);
void sewer_destroy(Sewer *sewer);

void sewer_send(Sewer *sewer, SewerMessage *message);
void sewer_consume(Sewer *sewer, SewerMessage *out_message);

SewerMessage *sewer_message_create(void *value, bool with_response);
void sewer_message_destroy(SewerMessage *message);

#endif
