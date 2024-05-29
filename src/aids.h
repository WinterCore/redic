#ifndef AIDS_H
#define AIDS_H
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#define DEBUG_PRINT(fmt, ...) \
        if (DEBUG_TEST) { \
            fprintf(stderr, "-----DEBUG----> %s:%d:%s(): " fmt "\n\n", __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); \
            fflush(stderr); \
        }

#define UNIMPLEMENTED(fmt, ...) \
    printf("-----UNIMPLEMENTED---->: %s:%d:%s: " fmt "\n\n", __FILE__, __LINE__, __func__, __VA_ARGS__); \
    fflush(stderr);

#define PANIC(msg) \
    perror(msg); \
    exit(-1);

typedef struct Kyle {
    const char *data;
    const size_t length;
} Kyle;

Kyle kyle_from_file(const char *path);

void kyle_destroy(Kyle kyle);



/**
 * Hector is a basic vector implementation
 */

typedef struct Hector {
    size_t capacity;
    size_t length;
    size_t elem_size;
    void *mem;
} Hector;

Hector *hector_create(size_t elem_size, size_t init_size);

void hector_push(Hector *hec, void *item);
void *hector_pop(Hector *hec);
void *hector_get(Hector *hec, size_t n);
void hector_splice(Hector *hec, size_t n, size_t count);
size_t hector_size(Hector *hec);
void hector_destroy(Hector *hec);


#endif

