#include "sewer.h"
#include "ring.h"
#include <pthread.h>
#include <stdlib.h>

Sewer *sewer_create(
    size_t cap,
    size_t elem_size
) {
    Sewer *sewer = malloc(sizeof(Sewer));

    pthread_mutex_init(&sewer->mutex, NULL);
    pthread_cond_init(&sewer->condvar, NULL);
    sewer->buffer = ringbuf_create(cap, elem_size);

    return sewer;
}

void sewer_send(Sewer *sewer, void *elem);
void sewer_consume(Sewer *sewer, void *out);

