#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "./data.h"
#include "./aids.h"


DataEntry *data_create_string_entry(
    OptionTime expires_at,
    char *str
) {
    DataEntry *entry = malloc(
        sizeof(DataEntry) +
        sizeof(DataString) +
        strlen(str)
    );
    
    entry->type = DATA_STRING;
    entry->expires_at = expires_at;

    DataString *data_str = (void *) entry->value;

    // TODO: Don't use strlen here, because the value is supposed to be bulk string
    // and might contain null characters. We should get the length as an argument
    data_str->len = strlen(str);
    memcpy(data_str->str, str, data_str->len);

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
