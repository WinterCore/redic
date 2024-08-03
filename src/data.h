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
    char *str;
    size_t len;
} DataString;

bool data_is_expired(DataEntry *entry);

DataEntry *data_create_string_entry(
    OptionTime expires_at,
    char *str
);

DataString *data_unwrap_string(DataEntry *entry);

void data_destroy_entry(DataEntry *entry);

#endif
