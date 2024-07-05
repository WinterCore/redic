#include <string.h>
#include <pthread.h>
#include <string.h>

#include "../../hashmap.h"
#include "../command.h"

RESPValue process_get(Arena *arena, Server *server, CommandArg **args);

CommandDefinition GET_COMMAND = COMMAND(
    "GET",
    ((CommandArgDefinition []) {
        COMMAND_ARGUMENT("key", ARG_TYPE_KEY, false),
    }),
    process_get
);

RESPValue process_get(Arena *arena, Server *server, CommandArg **args) {
    char *key = args[0]->value;


    pthread_mutex_lock(&server->data_lock);
    // TODO: Copy the value here
    void *value = NULL;
    int result = hashmap_get(server->data_map, key, &value);

    if (result == MAP_MISSING) {
        pthread_mutex_unlock(&server->data_lock);

        return resp_create_null_value(arena);
    }

    // Copy value
    size_t len = strlen(value);
    char *copy = arena_alloc(arena, len);
    memcpy(copy, value, len);
    pthread_mutex_unlock(&server->data_lock);


    return resp_create_bulk_string_value(arena, len, copy);
}
