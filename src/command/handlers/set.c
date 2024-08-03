#include <pthread.h>
#include <string.h>
#include <inttypes.h>

#include "../../hashmap.h"
#include "../../data.h"
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
    char *key = args[0]->value;
    char *value = args[1]->value;
    
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


    pthread_mutex_lock(&server->data_lock);

    DataEntry *old_value = NULL;

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
    OptionTime expires_at;

    switch (expiry_time.type) {
        case EXPIRY_DOESNT_EXPIRE: {
            expires_at.value = -1;
            expires_at.is_present = false;
            break;
        }
        case EXPIRY_UNIX_TS: {
            expires_at.value = expiry_time.ts;
            expires_at.is_present = true;
            break;
        }
        case EXPIRY_KEEP_OLD: {
            if (old_value != NULL) {
                expires_at = old_value->expires_at;
            } else {
                expires_at.value = -1;
                expires_at.is_present = false;
            }
            break;
        }
        default: {
            UNREACHABLE();
        }
    }

    DataEntry *entry = data_create_string_entry(expires_at, value);

    hashmap_put(server->data_map, key, entry);


    RESPValue returnValue;
    
    if (get) {
        if (old_value == NULL) {
            returnValue = resp_create_null_value(arena);
        } else {
            DataString *old_str_value = data_unwrap_string(old_value);
            char *value = arena_alloc(arena, old_str_value->len);
            strcpy(value, old_str_value->str);

            returnValue = resp_create_bulk_string_value(arena, old_str_value->len, value);
        }
    } else {
        returnValue = resp_create_simple_string_value(arena, "OK");
    }

    pthread_mutex_unlock(&server->data_lock);
    return returnValue;
}
