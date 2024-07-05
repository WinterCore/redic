#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>

#include "../resp/resp.h"
#include "../server.h"
#include "../arena.h"

#define COMMAND_ARGUMENT(NAME, TYPE, IS_OPTIONAL) \
    ((CommandArgDefinition) { .name = NAME, .type = TYPE, .is_optional = IS_OPTIONAL })

#define COMMAND(CMD_NAME, ARGS, HANDLER_FN) \
    (CommandDefinition) { \
        .name = CMD_NAME, \
        .args_len = sizeof(ARGS) / sizeof(ARGS[0]), \
        .args = ARGS, \
        .processor = HANDLER_FN, \
    }

typedef enum CommandArgType {
    ARG_TYPE_STRING,
    ARG_TYPE_INTEGER,
    ARG_TYPE_DOUBLE,
    ARG_TYPE_KEY,
    // ...
} CommandArgType;

typedef struct CommandArgDefinition {
    char *name;
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

    RESPValue (*processor)(Arena *arena, Server *server, CommandArg **args);
} CommandDefinition;

RESPValue process_command(Arena *arena, Server *server, RESPValue *input);

typedef enum CommandArgumentsParseResult {
    CMD_ARGS_PARSE_SUCCESS,
    CMD_ARGS_TOO_FEW_ARGS,
    CMD_ARGS_TOO_MANY_ARGS,
    CMD_ARGS_TYPE_MISMATCH,
} CommandArgumentsParseResult;

CommandArgumentsParseResult parse_command_arguments(
    Arena *arena,
    size_t arg_definitions_len,
    CommandArgDefinition arg_definitions[],
    size_t input_args_len,
    RESPBulkString **input_args,
    CommandArg **output_command_args
);


/**
 * Commands
 */
extern CommandDefinition PING_COMMAND;
extern CommandDefinition SET_COMMAND;
extern CommandDefinition GET_COMMAND;

#endif
