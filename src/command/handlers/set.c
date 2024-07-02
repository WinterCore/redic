#include "../command.h"

RESPValue process_set(Arena *arena, Server *server, CommandArg **args);

CommandDefinition PING_COMMAND = COMMAND(
    "PING",
    2,
    &((CommandArgDefinition []) {
        COMMAND_ARGUMENT("key", ARG_TYPE_KEY, false),
        COMMAND_ARGUMENT("value", ARG_TYPE_STRING, false),
        COMMAND_ARGUMENT("message", ARG_TYPE_STRING, false)
    })[0],
    process_set
);
