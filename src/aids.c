#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    // TODO: Check for malloc errors
    Hector *hector = (Hector*) arena_alloc(arena, sizeof(Hector));
    hector->elem_size = elem_size;
    hector->length = 0;
    hector->capacity = init_size;
    void *mem = arena_alloc(arena, elem_size * init_size);
    hector->mem = mem;

    return hector;
}


void hector_push(Hector *hec, void *item) {
    if (hec->capacity == hec->length) {
        int new_capacity = hec->capacity * 2;
        hec->mem = realloc(hec->mem, new_capacity * hec->elem_size);
        hec->capacity = new_capacity;
    }

    memcpy(
        hec->mem + (hec->length * hec->elem_size),
        &item,
        hec->elem_size
    );
    hec->length += 1;
}

void try_shrink(Hector *hec) {
    if (hec->length < hec->capacity / 3) {
        int new_capacity = hec->capacity / 2;
        hec->mem = realloc(hec->mem, new_capacity * hec->elem_size);
        hec->capacity = new_capacity;
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
    return *(void **)(hec->mem + (hec->elem_size * n));
}

size_t hector_size(Hector *hec) {
    return hec->length;
}

void hector_destroy(Hector *hec) {
    free(hec->mem);
    free(hec);
}

