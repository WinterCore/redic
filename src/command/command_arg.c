#include <string.h>

#include "./command.h"

typedef struct CommandArgumentsProcessor {
    size_t args_len;
    RESPBulkString **args;

    bool resolved_args[];
} CommandArgumentsProcessor;

CommandArgumentsProcessor *cap_create(
    Arena *arena,
    size_t input_args_len,
    RESPBulkString **input_args
) {
    CommandArgumentsProcessor *cap = arena_alloc(arena, sizeof(CommandArgumentsProcessor) + (input_args_len * sizeof(bool)));
    cap->args = input_args;
    cap->args_len = input_args_len;
    memset(cap->resolved_args, 0, input_args_len * sizeof(bool));
    


    return cap;
}

CommandArgumentsParseResult parse_command_argument(
    Arena *arena,
    CommandArgDefinition *arg_def,
    CommandArgumentsProcessor *cap,
    CommandArg **output_command_arg
) {
    for (size_t i = 0; i < cap->args_len; i += 1) {
        if (cap->resolved_args[i]) {
            continue;
        }

        RESPBulkString *arg = cap->args[i];
        RESPBulkString *token_arg = NULL;

        // Current arg def requires a token
        if (arg_def->token != NULL) {
            // Current argument does not match the token
            if (strcmp(arg_def->token, arg->data) != 0) {
                continue;
            }
            // Token matches, advance to next arg and save token arg
            token_arg = arg;
            arg = cap->args[i + 1];

            if (arg == NULL) {
                return CMD_ARGS_TOO_FEW_ARGS;
            }
        }

        // Early error return; Field is required and there's no value
        if (! arg_def->is_optional && arg == NULL) {
            return CMD_ARGS_TOO_FEW_ARGS;                    
        }

        CommandArg *output_arg = arena_alloc(arena, sizeof(CommandArg));
        output_arg->definition = arg_def;
        *output_command_arg = output_arg;

        void **value = &output_arg->value;

        if (arg_def->is_optional) {
            Option *option = arena_alloc(arena, sizeof(Option));
            option->is_present = arg != NULL;
            output_arg->value = option;
            
            // Early success return; Field is optional and there's no value
            if (arg == NULL) {
                break;
            }

            value = &option->value;
        }

        switch (arg_def->type) {
            case ARG_TYPE_STRING:
            case ARG_TYPE_KEY: {
                char *str = arena_alloc(arena, arg->length + 1);
                memcpy(str, arg->data, arg->length);
                str[arg->length] = '\0';

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
            case ARG_TYPE_UNIX_TIME: {
                UNIMPLEMENTED("Unimplemented arg parser for ARG_TYPE_UNIX_TIME %s", "");
                break;
            }
            case ARG_TYPE_ONEOF: {
                UNIMPLEMENTED("Unimplemented arg parser for ARG_TYPE_ONEOF %s", "");
                break;
            }
            case ARG_TYPE_PURE_TOKEN: {
                UNIMPLEMENTED("Unimplemented arg parser for ARG_TYPE_PURE_TOKEN %s", "");
                break;
            }
        }

        cap->resolved_args[i] = true;

        if (token_arg) {
            cap->resolved_args[i + 1] = true;
        }

        return CMD_ARGS_PARSE_SUCCESS;
    }

    if (arg_def->is_optional) {
        CommandArg *output_arg = arena_alloc(arena, sizeof(CommandArg));
        output_arg->definition = arg_def;

        *output_command_arg = output_arg;
        Option *option = arena_alloc(arena, sizeof(Option));
        option->is_present = false;
        output_arg->value = option;
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
    CommandArgumentsProcessor *cap = cap_create(
        arena,
        input_args_len,
        input_args
    );

    for (size_t i = 0; i < arg_definitions_len; i += 1) {
        CommandArgDefinition *arg_def = &arg_definitions[i];

        CommandArgumentsParseResult result = parse_command_argument(
            arena,
            arg_def,
            cap,
            &output_command_args[i]
        ); 

        if (result != CMD_ARGS_PARSE_SUCCESS) {
            return result;
        }
    }
    
    return CMD_ARGS_PARSE_SUCCESS;
}
