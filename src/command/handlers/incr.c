#include "../../septic_tank/septic_tank_operation.h"
#include "../command.h"

RESPValue process_incr(Arena *arena, Server *server, CommandArg **args);

CommandDefinition INCR_COMMAND = COMMAND(
    "INCR",
    COMMAND_ARGS(
        COMMAND_ARG("key", ARG_TYPE_STRING, false, NULL),
    ),
    process_incr
);

RESPValue process_incr(Arena *arena, Server *server, CommandArg **args) {
    DataString *key = args[0]->value;

    SepticTankOperation operation = {};
    operation.response_arena = arena;
    operation.type = SEPTIC_TANK_INCRBY;
    operation.incrby = (SepticTankIncrByOperation) { .key = key, .delta = 1 };

    SepticTankResult *result = septic_tank_feed(server->septic_tank_sewer, &operation);

    if (!result->success) {
        if (result->error != NULL) {
            return resp_create_simple_error_value(arena, result->error);
        }

        return resp_create_null_value(arena);
    }

    return resp_create_integer_value(arena, result->incrby_result.value);
}
