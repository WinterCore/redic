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
    CommandArgDefinition *arg_definitions,
    size_t input_args_len,
    RESPBulkString **input_args,
    CommandArg *output_command_args
) {
    for (size_t i = 0; i < arg_definitions_len; i += 1) {
        CommandArgDefinition def = arg_definitions[i];
    }

    UNIMPLEMENTED("parse_command_arguments %s", "");
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
            CommandArg *parsed_args = arena_alloc(arena, sizeof(CommandArg) * args_len);

            parse_command_arguments(
                arena,
                command_def->args_len,
                &command_def->args,
                args_len,
                input_args,
                parsed_args
            );

            // TODO: Implement argument parsing
            return command_def->processor(arena, server, NULL);
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

        for (size_t i = 0; i < array->array->length; i += 1) {
            RESPBulkString *value = ((RESPValue *) hector_get(array->array, i))->value;

            args[i] = value;
        }

        return process_command_helper(arena, server, command, args, args_len);
    }


    UNIMPLEMENTED("Inline command support %s", "");
}
