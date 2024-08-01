#include <string.h>
#include <pthread.h>
#include <string.h>

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
    MapEntry *value = NULL;

    int result = hashmap_get(server->data_map, key, (void **) &value);

    if (result == MAP_MISSING) {
        pthread_mutex_unlock(&server->data_lock);

        return resp_create_null_value(arena);
    }

    if (value->expires) {
        DEBUG_PRINT("EXPIRY(unix timestamp): %lld", (long long) value->expire_at);

        time_t now = time(NULL);
        double diff = value->expire_at - now;
        
        DEBUG_PRINT("EXPIRES IN(seconds): %f", diff);
    }

    // Copy value
    size_t len = strlen(value->value);
    char *copy = arena_alloc(arena, len);
    memcpy(copy, value->value, len);
    pthread_mutex_unlock(&server->data_lock);


    return resp_create_bulk_string_value(arena, len, copy);
}
