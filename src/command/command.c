#include <string.h>

#include "./command.h"
#include "../resp/resp.h"
#include "../arena.h"

static CommandDefinition* COMMANDS[] = {
    &PING_COMMAND,
    &SET_COMMAND,
    &GET_COMMAND,
};


bool is_valid_bulk_string_array_command(RESPArray *array) {
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
    size_t args_len,
    RESPBulkString **input_args
) {
    size_t commands_len = sizeof(COMMANDS) / sizeof(CommandDefinition *);

    for (size_t i = 0; i < commands_len; i += 1) {
        CommandDefinition *command_def = COMMANDS[i];

        // DEBUG_PRINT("Checking command %s %d", command->name, strcmp(command->name, input_string));
    
        if (strcmp(command_def->name, input_command->data) == 0) {
            CommandArg **parsed_args = arena_alloc(arena, sizeof(CommandArg *) * command_def->args_len);

            CommandArgumentsParseResult parse_args_result = parse_command_arguments(
                arena,
                command_def->args_len,
                command_def->args,
                args_len,
                input_args,
                parsed_args
            );
            
            if (parse_args_result != CMD_ARGS_PARSE_SUCCESS) {
                // TODO: Include more details in the error
                return resp_create_simple_error_value(arena, "Invalid command arguments");
            }

            // TODO: Implement argument parsing
            return command_def->processor(arena, server, parsed_args);
        }
    }

    return resp_create_simple_error_value(arena, "UNKNOWN COMMAND");
}


// TODO: Implement inline commands
RESPValue process_command(Arena *arena, Server *server, RESPValue *input) {
    if (input->kind == RESP_ARRAY) {
        RESPArray *array = input->value;

        if (! is_valid_bulk_string_array_command(array)) {
            UNIMPLEMENTED("Invalid command bulk string array %s", "");
        }

        RESPBulkString *command = ((RESPValue *) hector_get(array->array, 0))->value;
        size_t args_len = array->array->length - 1;
        RESPBulkString **args = arena_alloc(arena, sizeof(RESPBulkString *) * args_len);

        for (size_t i = 1; i < array->array->length; i += 1) {
            RESPBulkString *value = ((RESPValue *) hector_get(array->array, i))->value;

            args[i - 1] = value;
        }

        return process_command_helper(arena, server, command, args_len, args);
    }


    UNIMPLEMENTED("Inline command support %s", "");
}
