#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>

#include "../resp/resp.h"
#include "../server.h"

#define COMMAND_ARGUMENT(TYPE, IS_OPTIONAL) \
    ((CommandArgDefinition ) { .type = TYPE, .is_optional = IS_OPTIONAL })

#define COMMAND(CMD_NAME, ARGS_LEN, ARGS, HANDLER_FN) \
    (CommandDefinition) { \
        .name = CMD_NAME, \
        .args_len = ARGS_LEN, \
        .args = ARGS, \
    }

typedef enum CommandArgType {
    STRING,
    INEGER,
    DOUBLE,
    KEY,
    // ...
} CommandArgType;

typedef struct CommandArgDefinition {
    CommandArgType type;
    bool is_optional;
} CommandArgDefinition;

typedef struct CommandDefinition {
    char *name;

    size_t args_len;
    CommandArgDefinition *args;
} CommandDefinition;

typedef struct Option {
    bool is_present;
    void *value;
} Option;

typedef struct CommandArg {
    CommandDefinition *definition;

    void *value;
} CommandArg;

extern CommandDefinition PING_COMMAND;

RESPValue process_command(Server *server, char *command);

#endif
