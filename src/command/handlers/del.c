#include "../../sewer.h"
#include "../../septic_tank/septic_tank_operation.h"
#include "../command.h"

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

    Arena *message_arena = arena_create();

    SepticTankOperation *operation = arena_alloc(message_arena, sizeof(SepticTankOperation));
    operation->response_arena = arena;
    operation->type = SEPTIC_TANK_DEL;
    operation->del = (SepticTankDelOperation) { .key = key };
    SewerMessage *message = sewer_message_create(message_arena, operation, true);

    SepticTankResult *result = septic_tank_feed(server->septic_tank_sewer, message);

    if (result->success == false) {
        if (result->error != NULL) {
            return resp_create_simple_error_value(arena, result->error);
        }

        return resp_create_null_value(arena);
    }

    return resp_create_integer_value(arena, result->del_result.num_deleted);
}
