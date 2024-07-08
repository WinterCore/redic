#include <string.h>

#include "./command.h"

typedef struct CommandArguments {
    RESPBulkString **args;
    size_t len;
    size_t pos;
} CommandArguments;

CommandArguments command_arguments_create(size_t len, RESPBulkString **input_args) {
    CommandArguments cmd_args = {
        .args = input_args,
        .len = len,
        .pos = 0,
    };

    return cmd_args;
}

RESPBulkString *command_arguments_consume(CommandArguments *cmd_args) {
    if (cmd_args->pos == cmd_args->len) {
        return NULL;
    }

    return cmd_args->args[cmd_args->pos++];
}

CommandArgumentsParseResult parse_command_argument(
    Arena *arena,
    CommandArgDefinition *arg_definition,
    CommandArguments *cmd_args,
    CommandArg **output_command_arg
) {
    // TODO: If the argument has a token we need to consume to input arguments
    RESPBulkString *token_arg = NULL;

    // Token is required but there's no more input args
    if (arg_definition->token != NULL) {
        if ((token_arg = command_arguments_consume(cmd_args)) == NULL) {
            return CMD_ARGS_TOO_FEW_ARGS;
        }
    }


    // Early error return; Field is required and there's no value
    if (! arg_definition->is_optional && input_arg == NULL) {
        return CMD_ARGS_TOO_FEW_ARGS;                    
    }

    CommandArg *output_arg = arena_alloc(arena, sizeof(CommandArg));
    output_arg->definition = arg_definition;
    *output_command_arg = output_arg;

    void **value = &output_arg->value;

    if (arg_definition->is_optional) {

        Option *option = arena_alloc(arena, sizeof(Option));
        option->is_present = input_arg != NULL;
        output_arg->value = option;
        
        // Early success return; Field is optional and there's no value
        if (input_arg == NULL) {
            return CMD_ARGS_PARSE_SUCCESS;
        }

        value = &option->value;
    }

    switch (arg_definition->type) {
        case ARG_TYPE_STRING:
        case ARG_TYPE_KEY: {
            char *str = arena_alloc(arena, input_arg->length + 1);
            memcpy(str, input_arg->data, input_arg->length);
            str[input_arg->length] = '\0';

            *value = str;

            break;
        }
        case ARG_TYPE_INTEGER: {
            UNIMPLEMENTED("Unimplemented arg parser for ARG_TYPE_INTEGER %s", "");
            break;
        }
        case ARG_TYPE_DOUBLE: {
            UNIMPLEMENTED("Unimplemented arg parser for ARG_TYPE_DOUBLE %s", "");
            break;
        }
    }

    return CMD_ARGS_PARSE_SUCCESS;
}

CommandArgumentsParseResult parse_command_arguments(
    Arena *arena,
    size_t arg_definitions_len,
    CommandArgDefinition arg_definitions[],
    size_t input_args_len,
    RESPBulkString **input_args,
    CommandArg **output_command_args
) {
    CommandArguments cmd_args = command_arguments_create(input_args_len, input_args);

    // TODO: Handle too many arguments
    // TODO: Flip the logic here and iterate over arg definitions instead
    for (size_t i = 0; i < arg_definitions_len; i += 1) {
        CommandArgDefinition def = arg_definitions[i];
        RESPBulkString *input_arg = i < input_args_len
            ? input_args[i]
            : NULL;


        // TODO: If the argument is a token then search and destroy
        CommandArgumentsParseResult result = parse_command_argument(
            arena,
            &def,
            &cmd_args,
            &output_command_args[i]
        ); 

        if (result != CMD_ARGS_PARSE_SUCCESS) {
            return result;
        }
    }


    // GET THE NUMBER OF REQUIRED ARGUMENTS
    size_t num_args_required = 0;

    for (size_t i = 0; i < arg_definitions_len; i += 1) {
        if (! arg_definitions[i].is_optional) {
            num_args_required += 1;
        }
    }

    while (cmd_args.pos < cmd_args.len) {
        // Iterate over all required arguments in arg_definitions and make sure we fulfill them first
        
    }

    return CMD_ARGS_PARSE_SUCCESS;
}
