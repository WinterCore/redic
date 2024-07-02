#include <string.h>

#include "./command.h"
#include "../resp/resp.h"
#include "../arena.h"

static CommandDefinition* COMMANDS[] = {
    &PING_COMMAND,
};

CommandArgumentsParseResult parse_command_arguments(
    Arena *arena,
    size_t arg_definitions_len,
    CommandArgDefinition arg_definitions[],
    size_t input_args_len,
    RESPBulkString **input_args,
    CommandArg **output_command_args
) {
    // TODO: Handle too many arguments
    DEBUG_PRINT("PARSING ARGS %s", "");
    for (size_t i = 0; i < arg_definitions_len; i += 1) {
        CommandArgDefinition def = arg_definitions[i];
        RESPBulkString *input_arg = i < input_args_len
            ? input_args[i]
            : NULL;

        DEBUG_PRINT("ARG %p", input_arg);

        if (def.type == ARG_TYPE_STRING) {
            CommandArg *output_arg = arena_alloc(arena, sizeof(CommandArg));
            output_arg->definition = &def;

            if (def.is_optional) {
                Option *option = arena_alloc(arena, sizeof(Option));
                option->is_present = input_arg != NULL;
                output_arg->value = option;

                char *str = arena_alloc(arena, input_arg->length + 1);
                memcpy(str, input_arg->data, input_arg->length);
                str[input_arg->length] = '\0';

                option->value = str;
            } else {
                if (input_arg == NULL) { // Required argument doesn't exist
                    return CMD_ARGS_TOO_FEW_ARGS;                    
                } else {
                    char *str = arena_alloc(arena, input_arg->length + 1);
                    memcpy(str, input_arg->data, input_arg->length);
                    str[input_arg->length] = '\0';

                    output_arg->value = str;
                }
            }
            output_command_args[i] = output_arg;
        } else {
            UNIMPLEMENTED("Unsupported argument type %d", def.type);
        }
        
    }

    return CMD_ARGS_PARSE_SUCCESS;
}


bool is_valid_command_array(RESPArray *array) {
    // Zero length
    if (array->array->length == 0) {
        return false;
    }

    for (size_t i = 0; i < array->array->length; i += 1) {
        RESPValue *value = hector_get(array->array, i);

        if (value->kind != RESP_BULK_STRING) {
            return false;
        }
    }

    return true;
}

RESPValue process_command_helper(
    Arena *arena,
    Server *server,
    RESPBulkString *input_command,
    RESPBulkString **input_args,
    size_t args_len
) {
    size_t commands_len = sizeof(COMMANDS) / sizeof(CommandDefinition *);

    for (size_t i = 0; i < commands_len; i += 1) {
        CommandDefinition *command_def = COMMANDS[i];

        // DEBUG_PRINT("Checking command %s %d", command->name, strcmp(command->name, input_string));
    
        if (strncmp(command_def->name, input_command->data, strlen(command_def->name)) == 0) {
            CommandArg **parsed_args = arena_alloc(arena, sizeof(CommandArg *) * args_len);

            CommandArgumentsParseResult parse_args_result = parse_command_arguments(
                arena,
                command_def->args_len,
                command_def->args,
                args_len,
                input_args,
                parsed_args
            );
            
            if (parse_args_result != CMD_ARGS_PARSE_SUCCESS) {
                UNIMPLEMENTED("FAILED TO PARSE ARGS %d", parse_args_result);
            }

            // TODO: Implement argument parsing
            return command_def->processor(arena, server, parsed_args);
        }
    }

    return (RESPValue) { .kind = RESP_SIMPLE_ERROR, .value = NULL };
}


// TODO: Implement inline commands
RESPValue process_command(Arena *arena, Server *server, RESPValue *input) {
    if (input->kind == RESP_ARRAY) {
        RESPArray *array = input->value;

        if (! is_valid_command_array(array)) {
            UNIMPLEMENTED("Invalid command bulk string array %s", "");
        }

        RESPBulkString *command = ((RESPValue *) hector_get(array->array, 0))->value;
        size_t args_len = array->array->length - 1;
        RESPBulkString **args = arena_alloc(arena, sizeof(RESPBulkString *) * args_len);

        DEBUG_PRINT("LEN %zu", array->array->length);
        for (size_t i = 1; i < array->array->length; i += 1) {
            RESPBulkString *value = ((RESPValue *) hector_get(array->array, i))->value;

            args[i - 1] = value;
        }

        return process_command_helper(arena, server, command, args, args_len);
    }


    UNIMPLEMENTED("Inline command support %s", "");
}
