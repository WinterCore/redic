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

bool parse_long(char *str, long *value) {
    char *endptr;

    *value = strtol(str, &endptr, 10);
    
    return str != endptr;
}
