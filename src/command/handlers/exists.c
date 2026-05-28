#include "../../septic_tank/septic_tank_operation.h"
#include "../command.h"

RESPValue process_exists(Arena *arena, Server *server, CommandArg **args);

CommandDefinition EXISTS_COMMAND = COMMAND(
    "EXISTS",
    COMMAND_ARGS(
        COMMAND_ARG("key", ARG_TYPE_STRING, false, NULL)
    ),
    process_exists
);

RESPValue process_exists(Arena *arena, Server *server, CommandArg **args) {
    DataString *key = args[0]->value;

    SepticTankOperation operation = {};
    operation.response_arena = arena;
    operation.type = SEPTIC_TANK_EXISTS;
    operation.exists = (SepticTankExistsOperation) { .key = key };

    SepticTankResult *result = septic_tank_feed(server->septic_tank_sewer, &operation);

    return resp_create_integer_value(arena, result->exists_result.exists_count);
}
