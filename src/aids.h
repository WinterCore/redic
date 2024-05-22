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

struct Hector {
    void *mem;
    size_t capacity;
    size_t length;
    size_t elem_size;
};

struct Hector *hector_create(size_t elem_size, size_t init_size);

void hector_push(struct Hector *hec, void *item);
void *hector_pop(struct Hector *hec);
void *hector_get(struct Hector *hec, size_t n);
void hector_splice(struct Hector *hec, size_t n, size_t count);
size_t hector_size(struct Hector *hec);
void hector_destroy(struct Hector *hec);


#endif

