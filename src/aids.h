#ifndef AIDS_H
#define AIDS_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "./arena.h"

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#define UNUSED(x) (void)(x)

#define DEBUG_PRINT(fmt, ...) \
        if (DEBUG_TEST) { \
            fprintf(stderr, "-----DEBUG----> %s:%d:%s(): " fmt "\n\n", __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); \
            fflush(stderr); \
        }

#define UNIMPLEMENTED(fmt, ...) \
    printf("-----UNIMPLEMENTED---->: %s:%d:%s: " fmt "\n\n", __FILE__, __LINE__, __func__, __VA_ARGS__); \
    fflush(stderr); \
    exit(-1);

#define UNREACHABLE() \
    printf("-----UNREACHABLE---->: %s:%d:%s: \n\n", __FILE__, __LINE__, __func__); \
    fflush(stderr); \
    exit(-1);

#define PANIC(msg) \
    perror(msg); \
    fflush(stderr); \
    exit(-1);

typedef struct Kyle {
    const char *data;
    const size_t length;
} Kyle;

Kyle kyle_from_file(const char *path);

void kyle_destroy(Kyle kyle);


/**
 * Simple option implementation
 */
typedef struct Option {
    bool is_present;
    void *value;
} Option;

Option *option_create(Arena *arena, void *value);

#define optional_type(type) struct { bool is_present; type value; }

typedef optional_type(time_t) OptionTime;


/**
 * Hector is a basic vector implementation
 */

typedef struct Hector {
    size_t capacity;
    size_t length;
    size_t elem_size;
    void *mem;
} Hector;

Hector *hector_create(Arena *arena, size_t elem_size, size_t init_size);

void hector_push(Hector *hec, void *item);
void *hector_pop(Hector *hec);
void *hector_get(Hector *hec, size_t n);
void hector_splice(Hector *hec, size_t n, size_t count);
size_t hector_size(Hector *hec);
void hector_destroy(Hector *hec);


char *clone_string(Arena *arena, size_t str_len, char *str);

bool parse_long(char *str, long *value);

#endif

