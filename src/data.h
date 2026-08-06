#ifndef DATA_H
#define DATA_H

#include <time.h>
#include <stdbool.h>
#include <inttypes.h>

#include "./aids.h"

typedef struct DataEntry {
    OptionTime expires_at;

    enum DATA_TYPE {
        DATA_STRING,
    } type;

    uint8_t value[];
} DataEntry;

typedef struct DataString {
    size_t len;

    char str[];
} DataString;

/**
 * Current wall-clock time in unix milliseconds — the unit every expiry is stored in.
 */
int64_t data_now_ms(void);

bool data_is_expired(DataEntry *entry);

DataEntry *data_create_string_entry(
    OptionTime expires_at,
    size_t str_len,
    char *str
);

DataString *data_copy_string_arena(Arena *arena, DataString *string);
DataString *data_unwrap_string(DataEntry *entry);

/**
 * Allocates a DataString holding the base-10 representation of `value`.
 */
DataString *data_string_from_int64(Arena *arena, int64_t value);

void data_destroy_entry(DataEntry *entry);

#endif
