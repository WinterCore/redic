#include <pthread.h>

#include "../../septic_tank/septic_tank_operation.h"
#include "../command.h"

RESPValue process_ttl(Arena *arena, Server *server, CommandArg **args);

CommandDefinition TTL_COMMAND = COMMAND(
    "TTL",
    COMMAND_ARGS(
        COMMAND_ARG("key", ARG_TYPE_STRING, false, NULL)
    ),
    process_ttl
);

RESPValue process_ttl(Arena *arena, Server *server, CommandArg **args) {
    char *key = args[0]->value;

    SepticTankOperation operation = {};
    operation.response_arena = arena;
    operation.type = SEPTIC_TANK_TTL;
    operation.ttl = (SepticTankTtlOperation) { .key = key };

    SepticTankResult *result = septic_tank_feed(server->septic_tank_sewer, &operation);

    if (result->success == false) {
        if (result->error != NULL) {
            return resp_create_simple_error_value(arena, result->error);
        }

        return resp_create_null_value(arena);
    }

    return resp_create_integer_value(arena, result->ttl_result.ttl_s);
}
