#include "../command.h"
#include <stdio.h>
#include <string.h>

RESPValue process_info(Arena *arena, Server *server, CommandArg **args);

CommandDefinition INFO_COMMAND = COMMAND(
    "INFO",
    COMMAND_ARGS(),
    process_info
);

RESPValue process_info(Arena *arena, Server *server, CommandArg **args) {
    char *info = arena_alloc(arena, 1000);

    size_t n;
    
    if (server->master == NULL) {
        n = sprintf(info, "# Replication\nrole:master\nconnected_slaves:0\nmaster_replid:15\nmaster_repl_offset:0");
    } else {
        n = sprintf(info, "# Replication\nrole:replica\nconnected_slaves:0\nmaster_replid:15\nmaster_repl_offset:0");
    }

    RESPValue value = resp_create_bulk_string_value(arena, n + 1, info);
    
    return value;
}
