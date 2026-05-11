#include <pthread.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#include "../../sewer.h"
#include "../../septic_tank/septic_tank_operation.h"
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

SepticTankExpiration get_expire_time(CommandArg *arg) {
    Option *option = arg->value;

    if (! option->is_present) {
        return (SepticTankExpiration) { .ts = ST_EXPIRATION_NO_EXPIRE };
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int64_t now_ms = (int64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    if (strcmp(arg->definition->name, "seconds") == 0) {
        int64_t s = *((int64_t *) option->value);

        return (SepticTankExpiration) {
            .type = ST_EXPIRATION_UNIX_TS,
            .ts = now_ms + s * 1000,
        };
    } else if (strcmp(arg->definition->name, "milliseconds") == 0) {
        int64_t ms = *((int64_t *) option->value);

        return (SepticTankExpiration) {
            .type = ST_EXPIRATION_UNIX_TS,
            .ts = now_ms + ms,
        };
    } else if (strcmp(arg->definition->name, "unix-time-seconds") == 0) {
        int64_t unix_s = *((int64_t *) option->value);

        return (SepticTankExpiration) {
            .type = ST_EXPIRATION_UNIX_TS,
            .ts = unix_s * 1000,
        };
    } else if (strcmp(arg->definition->name, "unix-time-milliseconds") == 0) {
        int64_t unix_ms = *((int64_t *) option->value);

        return (SepticTankExpiration) {
            .type = ST_EXPIRATION_UNIX_TS,
            .ts = unix_ms,
        };
    } else if (strcmp(arg->definition->name, "keepttl") == 0) {
        return (SepticTankExpiration) {
            .type = ST_EXPIRATION_KEEP_OLD,
        };
    }

    UNREACHABLE("");
}

RESPValue process_set(Arena *arena, Server *server, CommandArg **args) {
    char *key = args[0]->value;
    char *value = args[1]->value;
    
    CommandArg *condition = args[2];
    bool is_condition_on = ((Option *) condition->value)->is_present;
    bool nx = is_condition_on && strcmp(condition->definition->name, "nx") == 0;
    bool xx = is_condition_on && strcmp(condition->definition->name, "xx") == 0;

    bool get = ((Option *) args[3]->value)->is_present;

    CommandArg *expiration = args[4];
    SepticTankExpiration expiry_time = get_expire_time(expiration);

    Arena *message_arena = arena_create();
    
    SepticTankOperation *operation = arena_alloc(message_arena, sizeof(SepticTankOperation));
    operation->response_arena = arena;
    operation->type = SEPTIC_TANK_SET;
    operation->set = (SepticTankSetOperation) {
        .expiration = expiry_time,
        .get = get,
        .xx = xx,
        .nx = nx,
        .key = key,
        .value = value,
    };
    SewerMessage *message = sewer_message_create(message_arena, operation, true);

    SepticTankResult *result = septic_tank_feed(server->septic_tank_sewer, message);

    if (result->success == false) {
        if (result->error != NULL) {
            return resp_create_simple_error_value(arena, result->error);
        }

        return resp_create_null_value(arena);
    }

    if (get) {
        if (result->set_result.old_value != NULL) {
            return resp_create_simple_string_value(
                arena,
                result->set_result.old_value->str
            );
        }

        return resp_create_null_value(arena);
    }
    

    return resp_create_simple_string_value(arena, "OK");
}
