#include <pthread.h>
#include <string.h>
#include <inttypes.h>

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
                COMMAND_ARG("unix-time-seconds", ARG_TYPE_UNIX_TIME, false, "EXAT"),
                COMMAND_ARG("unix-time-milliseconds", ARG_TYPE_UNIX_TIME, false, "PXAT"),
                COMMAND_ARG("keepttl", ARG_TYPE_PURE_TOKEN, false, "KEEPTTL"),
            )
        ),
    ),
    process_set
);

typedef struct ExpiryTime {
    enum { EXPIRY_DOESNT_EXPIRE, EXPIRY_UNIX_TS, EXPIRY_KEEP_OLD } type;

    time_t ts;
} ExpiryTime;

ExpiryTime get_expire_time(CommandArg *arg) {
    Option *option = arg->value;

    if (! option->is_present) {
        return (ExpiryTime) { .ts = EXPIRY_DOESNT_EXPIRE };
    }
    
    time_t time_s = time(NULL);

    if (strcmp(arg->definition->name, "seconds") == 0) {
        int64_t ms = *((int64_t *) option->value);

        DEBUG_PRINT("seconds %" PRId64, ms);

        return (ExpiryTime) {
            .type = EXPIRY_UNIX_TS,
            .ts = time_s + ms,
        };
    } else if (strcmp(arg->definition->name, "milliseconds") == 0) {
        int64_t ms = *((int64_t *) option->value) / 1000;

        return (ExpiryTime) {
            .type = EXPIRY_UNIX_TS,
            .ts = time_s + ms,
        };
    } else if (strcmp(arg->definition->name, "unix-time-seconds") == 0) {
        int64_t ts = *((int64_t *) option->value);

        return (ExpiryTime) {
            .type = EXPIRY_UNIX_TS,
            .ts = ts,
        };
    } else if (strcmp(arg->definition->name, "unix-time-milliseconds") == 0) {
        int64_t ts = *((int64_t *) option->value) / 1000;

        return (ExpiryTime) {
            .type = EXPIRY_UNIX_TS,
            .ts = ts,
        };
    } else if (strcmp(arg->definition->name, "keepttl") == 0) {
        return (ExpiryTime) {
            .type = EXPIRY_KEEP_OLD,
        };
    }

    UNREACHABLE();
}

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
    CommandArg *expiration = args[4];
    

    DEBUG_PRINT("NX %d", nx);
    DEBUG_PRINT("XX %d", xx);
    DEBUG_PRINT("GET %d", get);
    DEBUG_PRINT("EXPIRATION %d", ((Option *) args[4]->value)->is_present);

    MapEntry *entry = arena_alloc(server->arena, sizeof(MapEntry));

    entry->value = value;

    pthread_mutex_lock(&server->data_lock);

    MapEntry *old_value = NULL;

    if (nx || xx || get) {
        int result = hashmap_get(server->data_map, key, (void **) &old_value);

        if (
            (nx && result == MAP_OK) ||
            (xx && result == MAP_MISSING)
        ) {
            return resp_create_null_value(arena);
        }
    }

    ExpiryTime expiry_time = get_expire_time(expiration);

    switch (expiry_time.type) {
        case EXPIRY_DOESNT_EXPIRE: {
            entry->expire_at = -1;
            entry->expires = false;
            break;
        }
        case EXPIRY_UNIX_TS: {
            entry->expire_at = expiry_time.ts;
            entry->expires = true;
            break;
        }
        case EXPIRY_KEEP_OLD: {
            if (old_value != NULL) {
                entry->expire_at = old_value->expire_at;
                entry->expires = old_value->expires;
            } else {
                entry->expire_at = -1;
                entry->expires = false;
            }
            break;
        }
        default: {
            UNREACHABLE();
        }
    }

    hashmap_put(server->data_map, key, entry);

    if (get) {
        if (old_value == NULL) {
            pthread_mutex_unlock(&server->data_lock);
            return resp_create_null_value(arena);
        }

        size_t len = strlen(old_value->value);
        char *value = arena_alloc(arena, len);
        strcpy(value, old_value->value);

        pthread_mutex_unlock(&server->data_lock);
        return resp_create_bulk_string_value(arena, len, value);
    }

    pthread_mutex_unlock(&server->data_lock);
    return resp_create_simple_string_value(arena, "OK");
}
