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
        .processor = HANDLER_FN, \
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

typedef struct Option {
    bool is_present;
    void *value;
} Option;

typedef struct CommandArg {
    CommandArgDefinition *definition;

    void *value;
} CommandArg;

typedef struct CommandDefinition {
    char *name;

    size_t args_len;
    CommandArgDefinition *args;

    RESPValue (*processor)(Server *server, CommandArg *args);
} CommandDefinition;

extern CommandDefinition PING_COMMAND;

RESPValue process_command(Server *server, char *input_string);

#endif
