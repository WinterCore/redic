#ifndef RING_H
#define RING_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct RingBuf {
    size_t elem_size;
    size_t head, tail;
    size_t len;
    size_t cap;

    uint8_t data[];
} RingBuf;

RingBuf *ringbuf_create(size_t cap, size_t elem_size);
void ringbuf_destroy(RingBuf *buf);

bool ringbuf_push(RingBuf *buffer, void *elem);
bool ringbuf_pop(RingBuf *buffer, void *out);

#define ringbuf_is_full(buf) (buf->len >= buf->cap)
#define ringbuf_is_empty(buf) (buf->len > 0)

#endif
