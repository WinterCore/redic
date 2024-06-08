#include <string.h>

#include "./command.h"

static CommandDefinition* COMMANDS[] = {
    &PING_COMMAND,
};

void parse_command_arguments(
    CommandArgDefinition *arg_definitions,
    size_t arg_definitions_len,
    char *input_string,
    CommandArg *args,
    size_t *args_len
) {
    UNIMPLEMENTED("parse_command_arguments %s", "");
}

// TODO: Change input_string to array of strings which then we have to parse (this is because commands are sent as arrays of bulk string and we don't have to worry about splitting arguments from a raw string)
RESPValue process_command(Server *server, char *input_string) {
    size_t commands_len = sizeof(COMMANDS) / sizeof(CommandDefinition *);

    for (size_t i = 0; i < commands_len; i += 1) {
        CommandDefinition *command = COMMANDS[i];

        DEBUG_PRINT("Checking command %s %d", command->name, strcmp(command->name, input_string));

        // TODO: Find a better way to separate the args properly and trim the input string
        if (strncmp(command->name, input_string, strlen(command->name)) == 0) {
            // TODO: Implement argument parsing
            return command->processor(server, NULL);
        }
    }

    return (RESPValue) { .kind = RESP_SIMPLE_ERROR, .value = NULL };
}
