#include "../command.h"
#include <stdlib.h>


RESPValue process_ping(Arena *arena, Server *server, CommandArg *args);

CommandDefinition PING_COMMAND = COMMAND(
    "PING",
    1,
    ((CommandArgDefinition []) {
        COMMAND_ARGUMENT(ARG_TYPE_STRING, true)
    }),
    process_ping
);

RESPValue process_ping(Arena *arena, Server *server, CommandArg *args) {
    UNUSED(server);

    RESPSimpleString *pong = malloc(sizeof(RESPSimpleString));
    pong->string = "PONG";
    
    return (RESPValue) { .kind = RESP_SIMPLE_STRING, .value = pong  };
}
