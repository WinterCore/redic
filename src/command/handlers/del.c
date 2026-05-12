#include "../../septic_tank/septic_tank_operation.h"
#include "../command.h"
#include <string.h>

RESPValue process_del(Arena *arena, Server *server, CommandArg **args);

// TODO: Add support for deleting multiple keys (requires command parser changes)
CommandDefinition DEL_COMMAND = COMMAND(
    "DEL",
    COMMAND_ARGS(
        COMMAND_ARG("key", ARG_TYPE_STRING, false, NULL),
    ),
    process_del
);

RESPValue process_del(Arena *arena, Server *server, CommandArg **args) {
    char *key = args[0]->value;

    SepticTankOperation operation = {};
    operation.response_arena = arena;
    operation.type = SEPTIC_TANK_DEL;
    operation.del = (SepticTankDelOperation) { .key = key };

    SepticTankResult *result = septic_tank_feed(server->septic_tank_sewer, &operation);

    if (result->success == false) {
        if (result->error != NULL) {
            return resp_create_simple_error_value(arena, result->error);
        }
    }

    return resp_create_integer_value(arena, result->del_result.num_deleted);
}
