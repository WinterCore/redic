#include "../command.h"

CommandDefinition PING_COMMAND = COMMAND(
    "PING",
    1,
    ((CommandArgDefinition []) {
        COMMAND_ARGUMENT(STRING, true)
    }),
    process_ping
);

RESPValue process_ping(Server *server, CommandArg *args) {
}
