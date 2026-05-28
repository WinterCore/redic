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

#define DEBUG_PRINTF(...) \
        if (DEBUG_TEST) { \
            fprintf(stderr, "-----DEBUG----> %s:%d:%s(): ", __FILE__,  __LINE__, __func__); \
            fprintf(stderr, __VA_ARGS__); \
            fflush(stderr); \
        }

#define UNIMPLEMENTED(...) \
    printf("-----UNIMPLEMENTED---->: %s:%d:%s: ", __FILE__, __LINE__, __func__); \
    printf(__VA_ARGS__); \
    printf("\n\n"); \
    fflush(stderr); \
    exit(-1);

#define UNREACHABLE() \
    printf("-----UNREACHABLE---->: %s:%d:%s: \n\n", __FILE__, __LINE__, __func__); \
    printf("\n\n"); \
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

typedef optional_type(int64_t) OptionTime;


/**
 * Hector is a basic vector implementation
 */

typedef struct Hector {
    size_t capacity;
    size_t length;
    size_t elem_size;
    void *mem;
    Arena *arena;
} Hector;

Hector *hector_create(Arena *arena, size_t elem_size, size_t init_size);

void hector_push(Hector *hec, void *item);
void *hector_pop(Hector *hec);
void *hector_get(Hector *hec, size_t n);
void hector_splice(Hector *hec, size_t n, size_t count);
size_t hector_size(Hector *hec);
void hector_destroy(Hector *hec);


char *clone_string(Arena *arena, size_t str_len, char *str);
char *clone_cstr(Arena *arena, const char *str);

/**
 * Case-insensitive (ASCII) equality for two length-prefixed byte ranges.
 */
bool str_eq_ci(const char *a, size_t a_len, const char *b, size_t b_len);

bool parse_long(char *str, long *value);

/**
 * Parses exactly `len` bytes of `str` as a base-10 signed 64-bit integer.
 * Does not require null-termination. Returns false on empty input,
 * trailing junk, or over/underflow.
 */
bool parse_int64(const char *str, size_t len, int64_t *out);

/**
 * Returns current monotonic time in milliseconds.
 */
uint64_t monotonic_now_ms(void);

#endif

