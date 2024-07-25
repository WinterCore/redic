#include <pthread.h>
#include <string.h>

#include "../../hashmap.h"
#include "../command.h"

RESPValue process_set(Arena *arena, Server *server, CommandArg **args);

CommandArgDefinition foo = 
    COMMAND_ARG_ONE_OF(
        "condition",
        true,
        COMMAND_ARGS(
            COMMAND_ARG("nx", ARG_TYPE_PURE_TOKEN, false, "NX"),
            COMMAND_ARG("xx", ARG_TYPE_PURE_TOKEN, false, "XX"),
        )
    );

CommandDefinition SET_COMMAND = COMMAND(
    "SET",
    COMMAND_ARGS(
        COMMAND_ARG("key", ARG_TYPE_STRING, false, NULL),
        COMMAND_ARG("value", ARG_TYPE_STRING, false, NULL),
        COMMAND_ARG_ONE_OF(
            "condition",
            true,
            COMMAND_ARGS(
                COMMAND_ARG("nx", ARG_TYPE_PURE_TOKEN, false, "NX"),
                COMMAND_ARG("xx", ARG_TYPE_PURE_TOKEN, false, "XX"),
            )
        ),
        COMMAND_ARG("get", ARG_TYPE_PURE_TOKEN, true, "GET"),
        COMMAND_ARG_ONE_OF(
            "expiration",
            true,
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
    
    char *key = arena_alloc(server->arena, strlen(temp_key) + 1);
    char *value = arena_alloc(server->arena, strlen(temp_value) + 1);

    memcpy(key, temp_key, key_len + 1);
    memcpy(value, temp_value, value_len + 1);

    CommandArg *condition = args[2];
    bool is_condition_on = ((Option *) condition->value)->is_present;
    bool nx = strcmp(condition->definition->name, "nx") == 0 && is_condition_on;
    bool xx = strcmp(condition->definition->name, "xx") == 0 && is_condition_on;

    bool get = ((Option *) args[3]->value)->is_present;
    

    DEBUG_PRINT("NX %d", nx);
    DEBUG_PRINT("XX %d", xx);
    DEBUG_PRINT("GET %d", get);

    pthread_mutex_lock(&server->data_lock);
    hashmap_put(server->data_map, key, value);
    pthread_mutex_unlock(&server->data_lock);

    return resp_create_simple_string_value(arena, "OK");
}
