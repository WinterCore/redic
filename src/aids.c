#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

#include "aids.h"
#include "arena.h"

Kyle kyle_from_file(const char *path) {
    FILE *fd = fopen(path, "rb");

    if (fd == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    fseek(fd, 0, SEEK_END);
    size_t length = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    char *buffer = malloc(length + 1);

    if (buffer) {
        fread(buffer, 1, length, fd);
    }

    fclose(fd);

    Kyle fileData = {
        .data = buffer,
        .length = length,
    };

    return fileData;
}

void kyle_destroy(Kyle kyle) {
    free((void *) kyle.data);
}


Option *option_create(Arena *arena, void *value) {
    Option *opt = arena_alloc(arena, sizeof(Option));

    opt->is_present = value != NULL;
    opt->value = value;

    return opt;
}



Hector *hector_create(Arena *arena, size_t elem_size, size_t init_size) {
    Hector *hector = arena != NULL
        ? arena_alloc(arena, sizeof(Hector))
        : malloc(sizeof(Hector));
    hector->elem_size = elem_size;
    hector->length = 0;
    hector->capacity = init_size;
    hector->arena = arena;
    hector->mem = arena != NULL
        ? arena_alloc(arena, elem_size * init_size)
        : malloc(elem_size * init_size);

    return hector;
}

static void hector_resize(Hector *hec, size_t new_capacity) {
    if (hec->arena != NULL) {
        void *new_mem = arena_alloc(hec->arena, new_capacity * hec->elem_size);
        memcpy(new_mem, hec->mem, hec->length * hec->elem_size);
        hec->mem = new_mem;
    } else {
        hec->mem = realloc(hec->mem, new_capacity * hec->elem_size);
    }
    hec->capacity = new_capacity;
}

void hector_push(Hector *hec, void *item) {
    if (hec->capacity == hec->length) {
        hector_resize(hec, hec->capacity * 2);
    }

    memcpy(
        hec->mem + (hec->length * hec->elem_size),
        item,
        hec->elem_size
    );
    hec->length += 1;
}

void try_shrink(Hector *hec) {
    if (hec->length < hec->capacity / 3) {
        hector_resize(hec, hec->capacity / 2);
    }
}

void *hector_pop(Hector *hec) {
    void *value = hector_get(hec, hec->length - 1);
    hec->length -= 1;
    try_shrink(hec);

    return value;
}

void hector_splice(Hector *hec, size_t n, size_t count) {
    if (n + count > hec->capacity || count == 0) {
        return;
    }

    // Only move shit if it's in the middle
    if (n != hec->length - count) {
        memmove(
            hec->mem + (n * hec->elem_size),
            hec->mem + ((n + count) * hec->elem_size),
            hec->elem_size * (hec->length - n + count)
        );
    }

    hec->length -= count;
    try_shrink(hec);
}

void *hector_get(Hector *hec, size_t n) {
    if (n >= hec->length) {
        return NULL;
    }

    return hec->mem + (hec->elem_size * n);
}

size_t hector_size(Hector *hec) {
    return hec->length;
}

void hector_destroy(Hector *hec) {
    if (hec->arena != NULL) {
        return;
    }
    free(hec->mem);
    free(hec);
}

char *clone_string(Arena *arena, size_t str_len, char *str) {
    char *copy = arena_alloc(arena, str_len + 1);
    memcpy(copy, str, str_len);
    copy[str_len] = '\0';

    return copy;
}

char *clone_cstr(Arena *arena, const char *str) {
    size_t len = strlen(str) + 1;
    char *copy = arena != NULL ? arena_alloc(arena, len) : malloc(len);
    memcpy(copy, str, len);

    return copy;
}

bool str_eq_ci(const char *a, size_t a_len, const char *b, size_t b_len) {
    if (a_len != b_len) {
        return false;
    }

    for (size_t i = 0; i < a_len; i += 1) {
        char ca = a[i];
        char cb = b[i];

        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';

        if (ca != cb) {
            return false;
        }
    }

    return true;
}

bool parse_int64(const char *str, size_t len, int64_t *out) {
    // An int64 spans at most 20 chars (19 digits + optional sign)
    if (len == 0 || len > 20) {
        return false;
    }

    char buf[21];
    memcpy(buf, str, len);
    buf[len] = '\0';

    char *endptr;
    errno = 0;
    long long value = strtoll(buf, &endptr, 10);

    if (endptr != buf + len || errno == ERANGE) {
        return false;
    }

    *out = value;
    return true;
}

bool parse_long(char *str, long *value) {
    char *endptr;

    *value = strtol(str, &endptr, 10);
    
    return str != endptr;
}

uint64_t monotonic_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}
