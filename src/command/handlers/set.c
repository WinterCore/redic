#include <pthread.h>
#include <string.h>

#include "../../hashmap.h"
#include "../command.h"

RESPValue process_set(Arena *arena, Server *server, CommandArg **args);

CommandDefinition SET_COMMAND = COMMAND(
    "SET",
    COMMAND_ARGS(
        COMMAND_ARG("key", ARG_TYPE_STRING, false, NULL),
        COMMAND_ARG("value", ARG_TYPE_STRING, false, NULL),
        COMMAND_ARG_ONE_OF(
            "condition",
            true,
            NULL,
            COMMAND_ARGS(
                COMMAND_ARG("nx", ARG_TYPE_PURE_TOKEN, false, "NX"),
                COMMAND_ARG("xx", ARG_TYPE_PURE_TOKEN, false, "XX"),
            )
        ),
        COMMAND_ARG("get", ARG_TYPE_PURE_TOKEN, true, "GET"),
        COMMAND_ARG_ONE_OF(
            "expiration",
            true,
            NULL,
            COMMAND_ARGS(
                COMMAND_ARG("seconds", ARG_TYPE_INTEGER, false, "EX"),
                COMMAND_ARG("milliseconds", ARG_TYPE_INTEGER, false, "PX"),
                COMMAND_ARG("unix-time-seconds", ARG_TYPE_INTEGER, false, "EXAT"),
                COMMAND_ARG("unix-time-milliseconds", ARG_TYPE_INTEGER, false, "PXAT"),
                COMMAND_ARG("keepttl", ARG_TYPE_PURE_TOKEN, false, "KEEPTTL"),
            )
        ),
    ),
    process_set
);

RESPValue process_set(Arena *arena, Server *server, CommandArg **args) {
    char *temp_key = args[0]->value;
    char *temp_value = args[1]->value;
    size_t key_len = strlen(temp_key);
    size_t value_len = strlen(temp_value);
    
    char *key = malloc(strlen(temp_key));
    char *value = malloc(strlen(temp_value));

    if (key == NULL || value == NULL) {
        return resp_create_simple_error_value(arena, "Failed to allocate memory");
    }


    memcpy(key, temp_key, key_len);
    memcpy(value, temp_value, value_len);

    pthread_mutex_lock(&server->data_lock);
    hashmap_put(server->data_map, key, value);
    pthread_mutex_unlock(&server->data_lock);

    return resp_create_simple_string_value(arena, "OK");
}
