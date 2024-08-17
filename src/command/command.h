#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>

#include "../resp/resp.h"
#include "../server.h"
#include "../arena.h"

#define COMMAND_ARGS(...) \
    ((CommandArgDefinition []) { \
        __VA_ARGS__ \
    }) \

#define COMMAND_ARG(NAME, TYPE, IS_OPTIONAL, TOKEN) \
    ((CommandArgDefinition) { \
        .name = NAME, \
        .type = TYPE, \
        .is_optional = IS_OPTIONAL, \
        .token = TOKEN, \
    })

#define COMMAND_ARG_ONE_OF(NAME, IS_OPTIONAL, ARGS) \
    ((CommandArgDefinition) { \
        .name = NAME, \
        .type = ARG_TYPE_ONEOF, \
        .is_optional = IS_OPTIONAL, \
        .token = NULL, \
        .extra = { \
            .type = EXTRA_ARG_ARR, \
            .value = { \
                .arg_defs_arr =  { \
                    .arg_defs_len = sizeof(ARGS) / sizeof(ARGS[0]), \
                    .arg_defs = ARGS, \
                }, \
            } \
        } \
    })

#define COMMAND(CMD_NAME, ARGS, HANDLER_FN) \
    (CommandDefinition) { \
        .name = CMD_NAME, \
        .arg_defs_arr = { \
            .arg_defs_len = sizeof(ARGS) / sizeof(ARGS[0]), \
            .arg_defs = ARGS, \
        }, \
        .processor = HANDLER_FN, \
    }

typedef enum CommandArgType {
    ARG_TYPE_STRING,
    ARG_TYPE_INTEGER,
    ARG_TYPE_DOUBLE,
    ARG_TYPE_UNIX_TIME,
    ARG_TYPE_KEY,
    ARG_TYPE_ONEOF,
    ARG_TYPE_PURE_TOKEN,
    // ...
} CommandArgType;

typedef struct CommandArgDefinition CommandArgDefinition;

typedef struct CommandArgDefinitionArray {
    size_t arg_defs_len;
    CommandArgDefinition *arg_defs;
} CommandArgDefinitionArray;

typedef struct CommandArgDefinitionExtra {
    enum { EXTRA_ARG_ARR, EXTRA_DESCR } type;

    union {
        CommandArgDefinitionArray arg_defs_arr;
        char *description;
    } value;
} CommandArgDefinitionExtra;

typedef struct CommandArgDefinition {
    char *name;
    CommandArgType type;
    bool is_optional;

    char *token;

    CommandArgDefinitionExtra extra;
} CommandArgDefinition;

typedef struct CommandArg {
    CommandArgDefinition *definition;

    void *value;
} CommandArg;

typedef struct CommandDefinition {
    char *name;

    CommandArgDefinitionArray arg_defs_arr;
    RESPValue (*processor)(Arena *arena, Server *server, CommandArg **args);
} CommandDefinition;

RESPValue process_command(Arena *arena, Server *server, RESPValue *input);

typedef enum CommandArgParseResult {
    CMD_ARGS_PARSE_SUCCESS,
    CMD_ARGS_TOO_FEW_ARGS,
    CMD_ARGS_TOO_MANY_ARGS,
    CMD_ARGS_TOKEN_MISMATCH,
    CMD_ARGS_TYPE_MISMATCH,
} CommandArgParseResult;

CommandArgParseResult parse_command_arguments(
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
extern CommandDefinition INFO_COMMAND;

#endif
