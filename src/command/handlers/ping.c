#include "../command.h"
#include <string.h>


RESPValue process_ping(Arena *arena, Server *server, CommandArg **args);

CommandDefinition PING_COMMAND = COMMAND(
    "PING",
    COMMAND_ARGS(
        COMMAND_ARG("message", ARG_TYPE_STRING, true, NULL)
    ),
    process_ping
);

RESPValue process_ping(Arena *arena, Server *server, CommandArg **args) {
    UNUSED(server);

    Option *message_arg = (Option *) args[0]->value;

    if (message_arg->is_present) {
        return resp_create_bulk_string_value(arena, strlen(message_arg->value), message_arg->value);
    } else {
        return resp_create_simple_string_value(arena, "PONG");
    }
}
