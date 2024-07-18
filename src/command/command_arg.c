#include <string.h>

#include "./command.h"

typedef struct CommandArgProcessor {
    size_t args_len;
    RESPBulkString **args;

    bool resolved_args[];
} CommandArgProcessor;

typedef struct InputArgArray {
    size_t len;
    RESPBulkString **args;

    size_t start;
    size_t end;
} InputArgArray;

RESPBulkString *iaa_peek_arg(InputArgArray *iaa) {
    if (iaa->start < 0 || iaa->end <= iaa->start) {
        return NULL;
    }

    return iaa->args[iaa->start];
}

CommandArgProcessor *cap_create(
    Arena *arena,
    size_t input_args_len,
    RESPBulkString **input_args
) {
    CommandArgProcessor *cap = arena_alloc(arena, sizeof(CommandArgProcessor) + (input_args_len * sizeof(bool)));
    cap->args = input_args;
    cap->args_len = input_args_len;
    memset(cap->resolved_args, 0, input_args_len * sizeof(bool));
    
    return cap;
}

InputArgArray cap_get_aai_window(CommandArgProcessor *cap, Arena *arena) {
    size_t len = 0;
    size_t index = 0;

    for (size_t i = 0; i < cap->args_len; i += 1) {
        if (! cap->resolved_args[i]) {
            break;
        }

        index += 1;
    }

    for (size_t i = index; i < cap->args_len; i += 1) {
        if (cap->resolved_args[i]) {
            break;
        }

        len += 1;
    }

    InputArgArray arr = {
        .args = cap->args,
        .len = cap->args_len,
        .start = index,
        .end = index + len
    };

    return arr;
}

// This doesn't handle token
CommandArgParseResult parse_argument(
    Arena *arena,
    CommandArgDefinition *arg_def,
    InputArgArray *arg_arr,
    CommandArg **output_command_arg
) {
    RESPBulkString *arg = iaa_peek_arg(arg_arr);

    if (! arg_def->is_optional && arg == NULL) {
        return (CommandArgParseResult) {
            .type = CMD_ARGS_TOO_FEW_ARGS,
            .consumed_args = 0,
        };
    }

    CommandArg *output_arg = arena_alloc(arena, sizeof(CommandArg));
    output_arg->definition = arg_def;
    *output_command_arg = output_arg;

    void **value = &output_arg->value;

    if (arg_def->is_optional) {
        Option option = option_create(arg);

        output_arg->value = &option;
        
        // Early success return; Field is optional and there's no value
        if (arg == NULL) {
            return (CommandArgParseResult) {
                .type = CMD_ARGS_PARSE_SUCCESS,
                .consumed_args = 0,
            };
        }

        value = &option.value;
    }

    switch (arg_def->type) {
        case ARG_TYPE_STRING:
        case ARG_TYPE_KEY: {
            char *str = arena_alloc(arena, arg->length + 1);
            memcpy(str, arg->data, arg->length);
            str[arg->length] = '\0';

            *value = str;

            return (CommandArgParseResult) {
                .type = CMD_ARGS_PARSE_SUCCESS,
                .consumed_args = 1,
            };
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
            CommandArgDefinitionExtra *arg_def_extra = &arg_def->extra;
            
            if (arg_def_extra->type != EXTRA_ARG_ARR) {
                PANIC("ONEOF arg type must have an arg array in 'extra'");
            }

            UNIMPLEMENTED("Unimplemented arg parser for ARG_TYPE_ONEOF %s", "");
            // TODO: TO BE IMPLEMENTED
            break;
        }
        case ARG_TYPE_PURE_TOKEN: {
            static bool TRUE = true;
            *value = &TRUE;

            return (CommandArgParseResult) {
                .type = CMD_ARGS_PARSE_SUCCESS,
                .consumed_args = 1,
            };
        }
    }
}

CommandArgParseResult parse_command_argument(
    Arena *arena,
    CommandArgDefinition *arg_def,
    InputArgArray *arg_arr,
    CommandArg **output_command_arg
) {
    // Argument doesn't have a token
    if (! arg_def->token) {
        CommandArgParseResult result = parse_argument(
            arena,
            arg_def,
            arg_arr,
            output_command_arg
        );

        return result;
    }

    while (arg_arr->start < arg_arr->len) {
        RESPBulkString *arg = iaa_peek_arg(arg_arr);

        if (strcmp(arg->data, arg_def->token) == 0) {
            // Match
            CommandArgParseResult result = parse_argument(
                arena,
                arg_def,
                arg_arr,
                output_command_arg
            );

            return result;
        }

        arg_arr->start += 1;
    }
   
    UNIMPLEMENTED("COULDN'T FIND TOKEN MATCH ARG %s", "");
}

CommandArgParseResult parse_command_arguments(
    Arena *arena,
    size_t arg_definitions_len,
    CommandArgDefinition arg_definitions[],
    size_t input_args_len,
    RESPBulkString **input_args,
    CommandArg **output_command_args
) {
    CommandArgProcessor *cap = cap_create(
        arena,
        input_args_len,
        input_args
    );


    for (size_t i = 0; i < arg_definitions_len; i += 1) {
        CommandArgDefinition *arg_def = &arg_definitions[i];

        InputArgArray iaa = cap_get_aai_window(cap, arena);

        size_t index = iaa.start;

        CommandArgParseResult result = parse_argument(
            arena,
            arg_def,
            &iaa,
            &output_command_args[i]
        ); 
        DEBUG_PRINT("PARSED %zu", i);

        if (result.type != CMD_ARGS_PARSE_SUCCESS) {
            return result;
        }
        DEBUG_PRINT("NEXT %zu", i);
        
        for (size_t i = index; i < index + result.consumed_args; i += 1) {
            cap->resolved_args[i] = true;
        }
    }
    
    return (CommandArgParseResult) {
        .type = CMD_ARGS_PARSE_SUCCESS,
        .consumed_args = 1,
    };
}
