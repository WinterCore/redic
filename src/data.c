#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "./data.h"
#include "./aids.h"


DataEntry *data_create_string_entry(
    OptionTime expires_at,
    size_t str_len,
    char *str
) {
    DataEntry *entry = malloc(
        sizeof(DataEntry) +
        sizeof(DataString) +
        str_len
    );
    
    entry->type = DATA_STRING;
    entry->expires_at = expires_at;

    DataString *data_str = (void *) entry->value;

    data_str->len = str_len;
    memcpy(data_str->str, str, str_len);

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

    time_t expires_at = entry->expires_at.value;
    time_t now = time(NULL);

    return expires_at <= now;
}

void data_destroy_entry(DataEntry *entry) {
    free(entry);
}
