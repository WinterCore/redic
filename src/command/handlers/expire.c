#include <time.h>
#include <string.h>

#include "../../septic_tank/septic_tank_operation.h"
#include "../command.h"

RESPValue process_expire(Arena *arena, Server *server, CommandArg **args);

CommandDefinition EXPIRE_COMMAND = COMMAND(
    "EXPIRE",
    COMMAND_ARGS(
        COMMAND_ARG("key", ARG_TYPE_STRING, false, NULL),
        COMMAND_ARG("seconds", ARG_TYPE_INTEGER, false, NULL),
        COMMAND_ARG_ONE_OF(
            "condition",
            true,
            COMMAND_ARGS(
                COMMAND_ARG("nx", ARG_TYPE_PURE_TOKEN, false, "NX"),
                COMMAND_ARG("xx", ARG_TYPE_PURE_TOKEN, false, "XX"),
                COMMAND_ARG("gt", ARG_TYPE_PURE_TOKEN, false, "GT"),
                COMMAND_ARG("lt", ARG_TYPE_PURE_TOKEN, false, "LT"),
            )
        ),
    ),
    process_expire
);

static ExpireCondition resolve_expire_condition(CommandArg *condition) {
    if (! ((Option *) condition->value)->is_present) {
        return EXPIRE_COND_NONE;
    }

    char *name = condition->definition->name;

    if (strcmp(name, "nx") == 0) {
        return EXPIRE_COND_NX;
    } else if (strcmp(name, "xx") == 0) {
        return EXPIRE_COND_XX;
    } else if (strcmp(name, "gt") == 0) {
        return EXPIRE_COND_GT;
    } else if (strcmp(name, "lt") == 0) {
        return EXPIRE_COND_LT;
    }

    UNREACHABLE();
}

RESPValue process_expire(Arena *arena, Server *server, CommandArg **args) {
    DataString *key = args[0]->value;
    int64_t seconds = *((int64_t *) args[1]->value);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int64_t now_ms = (int64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    SepticTankOperation operation = {};
    operation.response_arena = arena;
    operation.type = SEPTIC_TANK_EXPIRE;
    operation.expire = (SepticTankExpireOperation) {
        .key = key,
        .expires_at = now_ms + seconds * 1000,
        .condition = resolve_expire_condition(args[2]),
    };

    SepticTankResult *result = septic_tank_feed(server->septic_tank_sewer, &operation);

    if (!result->success) {
        if (result->error != NULL) {
            return resp_create_simple_error_value(arena, result->error);
        }

        return resp_create_null_value(arena);
    }

    return resp_create_integer_value(arena, result->expire_result.is_set);
}
