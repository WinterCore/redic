#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "./data.h"
#include "./aids.h"
#include "arena.h"

DataString *data_copy_string_arena(Arena *arena, DataString *string) {
    size_t len = sizeof(DataString) + string->len + 1;
    DataString *arena_string = arena_alloc(arena, len);
    
    memcpy(arena_string, string, len);

    return arena_string;
}

DataEntry *data_create_string_entry(
    OptionTime expires_at,
    size_t str_len,
    char *str
) {
    DataEntry *entry = malloc(
        sizeof(DataEntry) +
        sizeof(DataString) +
        str_len + 1
    );
    
    entry->type = DATA_STRING;
    entry->expires_at = expires_at;

    DataString *data_str = (void *) entry->value;

    data_str->len = str_len;
    memcpy(data_str->str, str, str_len);
    data_str->str[str_len] = '\0';

    return entry;
}

DataString *data_unwrap_string(DataEntry *entry) {
    assert(entry->type == DATA_STRING);

    return (void *) entry->value;
}

bool data_is_expired(DataEntry *entry) {
    if (! entry->expires_at.is_present) {
        return false;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int64_t now_ms = (int64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    return entry->expires_at.value <= now_ms;
}

void data_destroy_entry(DataEntry *entry) {
    free(entry);
}
