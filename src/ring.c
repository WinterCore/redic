#include <stdlib.h>
#include <string.h>

#include "ring.h"

RingBuf *ringbuf_create(
    size_t cap,
    size_t elem_size
) {
    RingBuf *buff = malloc(sizeof(RingBuf) + cap * elem_size);
    buff->cap = cap;
    buff->elem_size = elem_size;
    buff->head = 0;
    buff->tail = 0;
    buff->len = 0;

    return buff;
}

void ringbuf_destroy(RingBuf *buf) {
    free(buf);
}

bool ringbuf_push(RingBuf *buffer, void *elem) {
    if (buffer->len >= buffer->cap) {
        return false;
    }

    memcpy(
        &buffer->data[buffer->tail * buffer->elem_size],
        elem,
        buffer->elem_size
    );

    buffer->len += 1;
    buffer->tail = (buffer->tail + 1) % buffer->cap;

    return true;
}

bool ringbuf_pop(RingBuf *buffer, void *out) {
    if (buffer->len == 0) {
        return false;
    }

    void *value = &buffer->data[buffer->head * buffer->elem_size];

    buffer->len -= 1;
    buffer->head = (buffer->head + 1) % buffer->cap;
    memcpy(out, value, buffer->elem_size);

    return true;
}
