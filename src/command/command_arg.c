#include <string.h>

#include "./command.h"

CommandArgumentsParseResult parse_command_argument(
    Arena *arena,
    CommandArgDefinition *arg_definition,
    RESPBulkString **input_args,
    CommandArg **output_command_arg,
    size_t *consumed_args
) {
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
    // TODO: Handle too many arguments
    for (size_t i = 0; i < arg_definitions_len;) {
        CommandArgDefinition def = arg_definitions[i];
        RESPBulkString *input_arg = i < input_args_len
            ? input_args[i]
            : NULL;

        size_t consumed_args = 0;

        // TODO: finish this
        CommandArgumentsParseResult result = parse_command_argument(
            arena,
            &def,
            input_args,
            &output_command_args[i],
            &consumed_args
        ); 

        i += consumed_args; 

        if (result != CMD_ARGS_PARSE_SUCCESS) {
            return result;
        }
    }

    return CMD_ARGS_PARSE_SUCCESS;
}
