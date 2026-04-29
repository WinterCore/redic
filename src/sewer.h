#ifndef SEWER_H
#define SEWER_H

#include <pthread.h>

#include "ring.h"

typedef struct Sewer {
    RingBuf *buffer;
    pthread_mutex_t mutex;
    pthread_cond_t condvar;
} Sewer;

Sewer *sewer_create(
    size_t cap,
    size_t elem_size
);

void sewer_send(Sewer *sewer, void *elem);
void sewer_consume(Sewer *sewer, void *out);

#endif
