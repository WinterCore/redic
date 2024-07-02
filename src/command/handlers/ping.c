#include "../command.h"
#include <string.h>


RESPValue process_ping(Arena *arena, Server *server, CommandArg **args);

CommandDefinition PING_COMMAND = COMMAND(
    "PING",
    1,
    &((CommandArgDefinition []) {
        COMMAND_ARGUMENT("message", ARG_TYPE_STRING, true),
    })[0],
    process_ping
);

RESPValue process_ping(Arena *arena, Server *server, CommandArg **args) {
    UNUSED(server);

    Option *message_arg = (Option *) args[0]->value;

    if (message_arg->is_present) {
        RESPBulkString *message = arena_alloc(arena, sizeof(RESPBulkString));
        message->data = message_arg->value;
        message->length = strlen(message_arg->value);
        return (RESPValue) { .kind = RESP_BULK_STRING, .value = message };
    } else {
        RESPSimpleString *pong = arena_alloc(arena, sizeof(RESPSimpleString));
        pong->string = message_arg->value;
        return (RESPValue) { .kind = RESP_SIMPLE_STRING, .value = pong };
    }
}
