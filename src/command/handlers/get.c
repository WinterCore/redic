#include <string.h>
#include <pthread.h>
#include <string.h>

#include "../../data.h"
#include "../../hashmap.h"
#include "../command.h"

RESPValue process_get(Arena *arena, Server *server, CommandArg **args);

CommandDefinition GET_COMMAND = COMMAND(
    "GET",
    COMMAND_ARGS(
        COMMAND_ARG("key", ARG_TYPE_STRING, false, NULL)
    ),
    process_get
);

RESPValue process_get(Arena *arena, Server *server, CommandArg **args) {
    char *key = args[0]->value;

    pthread_mutex_lock(&server->data_lock);
    // TODO: Copy the value here
    DataEntry *entry = NULL;

    int result = hashmap_get(server->data_map, key, (void **) &entry);

    if (result == MAP_MISSING) {
        pthread_mutex_unlock(&server->data_lock);

        return resp_create_null_value(arena);
    }

    if (data_is_expired(entry)) {
        hashmap_remove(server->data_map, key);
        data_destroy_entry(entry);
        return resp_create_null_value(arena);
    }

    // Copy value
    DataString *str_entry = data_unwrap_string(entry);
    char *copy = arena_alloc(arena, str_entry->len);
    memcpy(copy, str_entry->str, str_entry->len);
    pthread_mutex_unlock(&server->data_lock);


    return resp_create_bulk_string_value(arena, len, copy);
}
