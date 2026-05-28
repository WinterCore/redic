#include "../../septic_tank/septic_tank_operation.h"
#include "../command.h"

RESPValue process_incrby(Arena *arena, Server *server, CommandArg **args);

CommandDefinition INCRBY_COMMAND = COMMAND(
    "INCRBY",
    COMMAND_ARGS(
        COMMAND_ARG("key", ARG_TYPE_STRING, false, NULL),
        COMMAND_ARG("increment", ARG_TYPE_INTEGER, false, NULL),
    ),
    process_incrby
);

RESPValue process_incrby(Arena *arena, Server *server, CommandArg **args) {
    DataString *key = args[0]->value;
    int64_t increment = *((int64_t *) args[1]->value);

    SepticTankOperation operation = {};
    operation.response_arena = arena;
    operation.type = SEPTIC_TANK_INCRBY;
    operation.incrby = (SepticTankIncrByOperation) { .key = key, .delta = increment };

    SepticTankResult *result = septic_tank_feed(server->septic_tank_sewer, &operation);

    if (!result->success) {
        if (result->error != NULL) {
            return resp_create_simple_error_value(arena, result->error);
        }

        return resp_create_null_value(arena);
    }

    return resp_create_integer_value(arena, result->incrby_result.value);
}
