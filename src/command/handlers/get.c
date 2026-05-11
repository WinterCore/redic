#include <string.h>
#include <pthread.h>
#include <string.h>

#include "../../sewer.h"
#include "../../septic_tank/septic_tank_operation.h"
#include "../command.h"

RESPValue process_get(Arena *arena, Server *server, CommandArg **args);

CommandDefinition GET_COMMAND = COMMAND(
    "GET",
    COMMAND_ARGS(
        COMMAND_ARG("key", ARG_TYPE_STRING, false, NULL)
    ),
    process_get
);

RESPValue process_get(Arena *arena, Server *server, CommandArg **args) {
    char *key = args[0]->value;

    Arena *message_arena = arena_create();
    
    SepticTankOperation *operation = arena_alloc(message_arena, sizeof(SepticTankOperation));
    operation->response_arena = arena;
    operation->type = SEPTIC_TANK_GET;
    operation->get = (SepticTankGetOperation) { .key = key };
    SewerMessage *message = sewer_message_create(message_arena, operation, true);

    SepticTankResult *result = septic_tank_feed(server->septic_tank_sewer, message);

    // TODO: Handle error
    if (result->get_result.value == NULL) {
        return resp_create_null_value(arena);
    }

    DataString *string = result->get_result.value;

    return resp_create_bulk_string_value(arena, string->len, string->str);
}
