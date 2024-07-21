#include <assert.h>
#include <string.h>

#include "./command.h"

typedef struct InputArgArray {
    size_t args_len;
    RESPBulkString **args;

    size_t start;
    size_t end;

    bool resolved_args[];
} InputArgArray;

InputArgArray *iaa_create(
    Arena *arena,
    size_t args_len,
    RESPBulkString **args
) {
    InputArgArray *iaa = arena_alloc(arena, sizeof(InputArgArray) + (sizeof(bool) * args_len));
    iaa->args = args;
    iaa->args_len = args_len;
    memset(iaa->resolved_args, 0, args_len);
    iaa->start = 0;
    iaa->end = args_len;

    return iaa;
}

int iaa_peek_arg_index(InputArgArray *iaa, size_t index) {
    for (size_t i = iaa->start, j = 0; i < iaa->end; i += 1) {
        if (iaa->resolved_args[i]) {
            continue;
        }

        if (index == j) {
            return i;
        }

        j += 1;
    }
    
    return -1;
}


RESPBulkString *iaa_peek_arg(InputArgArray *iaa, size_t index) {
    int idx = iaa_peek_arg_index(iaa, index);
    
    return idx == -1 ? NULL : iaa->args[idx];
}

bool iaa_consume_arg(InputArgArray *iaa, size_t index) {
    int idx = iaa_peek_arg_index(iaa, index);
    
    if (idx == -1) {
        return false;
    }

    iaa->resolved_args[idx] = true;
    return true;
}

InputArgArray *iaa_clone_shrinked_window_at_index(Arena *arena, InputArgArray *iaa, size_t index) {
    int idx = iaa_peek_arg_index(iaa, index);
    
    assert(idx >= 0 && "iaa_clone_windowed: idx should be >= 0");

    // Clone existing iaa
    size_t bytes = sizeof(InputArgArray) + (sizeof(bool) * iaa->args_len);
    InputArgArray *iaa_shrinked = arena_alloc(arena, bytes);
    memcpy(iaa_shrinked, iaa, bytes);

    // TODO: I'm assuming ths is ok?
    size_t start = idx;

    assert((int) start <= idx && "iaa_clone_windowed: start should be <= idx");

    size_t end = start;
    while (! iaa->resolved_args[end]) {
        end += 1;
    }

    iaa_shrinked->start = start;
    iaa_shrinked->end = end;

    return iaa_shrinked;
}

CommandArgParseResult parse_argument(
    Arena *arena,
    CommandArgDefinition *arg_def,
    InputArgArray *iaa,
    CommandArg **output_command_arg
) {
    RESPBulkString *arg = iaa_peek_arg(iaa, 0);

    if (! arg_def->is_optional && arg == NULL) {
        return CMD_ARGS_TOO_FEW_ARGS;
    }

    CommandArg *output_arg = arena_alloc(arena, sizeof(CommandArg));
    output_arg->definition = arg_def;
    *output_command_arg = output_arg;

    void **value = &output_arg->value;

    if (arg_def->is_optional) {
        Option *option = option_create(arena, arg);

        output_arg->value = option;
        
        // Early success return; Field is optional and there's no value
        if (arg == NULL) {
            return CMD_ARGS_PARSE_SUCCESS;
        }

        value = &option->value;
    }

    switch (arg_def->type) {
        case ARG_TYPE_STRING:
        case ARG_TYPE_KEY: {
            iaa_consume_arg(iaa, 0);
            char *str = arena_alloc(arena, arg->length + 1);
            memcpy(str, arg->data, arg->length);
            str[arg->length] = '\0';

            *value = str;

            return CMD_ARGS_PARSE_SUCCESS;
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

            return CMD_ARGS_PARSE_SUCCESS;
        }
    }
}

CommandArgParseResult parse_command_argument(
    Arena *arena,
    CommandArgDefinition *arg_def,
    InputArgArray *iaa,
    CommandArg **output_command_arg
) {
    // Argument doesn't have a token
    if (! arg_def->token) {
        return parse_argument(
            arena,
            arg_def,
            iaa,
            output_command_arg
        );
    }
    
    RESPBulkString *arg = NULL;

    size_t i = 0;

    while ((arg = iaa_peek_arg(iaa, i))) {
        if (arg == NULL) {
            break;
        }

        if (strcmp(arg->data, arg_def->token) == 0) {
            iaa_consume_arg(iaa, i);

            InputArgArray *shrinked_iaa = iaa_clone_shrinked_window_at_index(arena, iaa, i);

            CommandArgParseResult result = parse_argument(
                arena,
                arg_def,
                shrinked_iaa,
                output_command_arg
            );

            return result;
        }

        i += 1;
    }

    if (arg_def->is_optional) {
        CommandArg *output_arg = arena_alloc(arena, sizeof(CommandArg));
        output_arg->definition = arg_def;
        *output_command_arg = output_arg;

        Option *option = option_create(arena, NULL);
        output_arg->value = option;

        return CMD_ARGS_PARSE_SUCCESS;
    }

    // TODO: Check if arg def is optional and return None
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
    InputArgArray *iaa = iaa_create(
        arena,
        input_args_len,
        input_args
    );

    for (size_t i = 0; i < arg_definitions_len; i += 1) {
        CommandArgDefinition *arg_def = &arg_definitions[i];

        CommandArgParseResult result = parse_command_argument(
            arena,
            arg_def,
            iaa,
            &output_command_args[i]
        ); 

        if (result != CMD_ARGS_PARSE_SUCCESS) {
            return result;
        }
    }
    
    return CMD_ARGS_PARSE_SUCCESS;
}
