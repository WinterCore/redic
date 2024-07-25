#include <assert.h>
#include <string.h>
#include <time.h>

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
    
    if (idx == -1) {
        idx = iaa->end;
    }
    
    // Clone existing iaa
    size_t bytes = sizeof(InputArgArray) + (sizeof(bool) * iaa->args_len);
    InputArgArray *iaa_shrinked = arena_alloc(arena, bytes);
    memcpy(iaa_shrinked, iaa, bytes);

    // TODO: I'm assuming ths is ok?
    size_t start = idx;

    size_t end = start;

    while (end < iaa->args_len && ! iaa->resolved_args[end]) {
        end += 1;
    }

    iaa_shrinked->start = start;
    iaa_shrinked->end = end;

    return iaa_shrinked;
}

CommandArgParseResult parse_command_argument(
    Arena *arena,
    CommandArgDefinition *arg_def,
    InputArgArray *iaa,
    CommandArg **output_command_arg
);

CommandArgParseResult parse_argument(
    Arena *arena,
    CommandArgDefinition *arg_def,
    InputArgArray *iaa,
    CommandArg **output_command_arg
) {
    RESPBulkString *arg = iaa_peek_arg(iaa, 0);
    CommandArg *output_arg = arena_alloc(arena, sizeof(CommandArg));
    output_arg->definition = arg_def;
    *output_command_arg = output_arg;

    void **value = &output_arg->value;

    Option *option = NULL;

    if (arg_def->is_optional) {
        option = option_create(arena, arg);
        output_arg->value = option;
        value = &option->value;
    }

    switch (arg_def->type) {
        case ARG_TYPE_STRING:
        case ARG_TYPE_KEY: {
            if (option == NULL && arg == NULL) {
                return CMD_ARGS_TOO_FEW_ARGS;
            }

            if (option != NULL && arg == NULL) {
                option->is_present = false;
                return CMD_ARGS_PARSE_SUCCESS;
            }

            iaa_consume_arg(iaa, 0);
            char *str = arena_alloc(arena, arg->length + 1);
            memcpy(str, arg->data, arg->length);
            str[arg->length] = '\0';

            *value = str;

            if (option != NULL) {
                option->is_present = true;
            }

            return CMD_ARGS_PARSE_SUCCESS;
        }
        case ARG_TYPE_INTEGER: {
            if (option == NULL && arg == NULL) {
                return CMD_ARGS_TOO_FEW_ARGS;
            }

            if (option != NULL && arg == NULL) {
                option->is_present = false;
                return CMD_ARGS_PARSE_SUCCESS;
            }

            // TODO: error handling
            int64_t *num = arena_alloc(arena, sizeof(int64_t));
            *num = strtoll(arg->data, NULL, 10);

            *value = num;

            if (option != NULL) {
                option->is_present = true;
            }

            return CMD_ARGS_PARSE_SUCCESS;
        }
        case ARG_TYPE_DOUBLE: {
            if (option == NULL && arg == NULL) {
                return CMD_ARGS_TOO_FEW_ARGS;
            }

            if (option != NULL && arg == NULL) {
                option->is_present = false;
                return CMD_ARGS_PARSE_SUCCESS;
            }

            // TODO: error handling
            int64_t *num = arena_alloc(arena, sizeof(int64_t));
            *num = strtod(arg->data, NULL);

            *value = num;

            if (option != NULL) {
                option->is_present = true;
            }

            return CMD_ARGS_PARSE_SUCCESS;
        }
        case ARG_TYPE_UNIX_TIME: {
            if (option == NULL && arg == NULL) {
                return CMD_ARGS_TOO_FEW_ARGS;
            }

            if (option != NULL && arg == NULL) {
                option->is_present = false;
                return CMD_ARGS_PARSE_SUCCESS;
            }

            long long int *num = arena_alloc(arena, sizeof(long long int));
            *num = strtoll(arg->data, NULL, 10);

            *value = num;

            if (option != NULL) {
                option->is_present = true;
            }

            return CMD_ARGS_PARSE_SUCCESS;
        }
        case ARG_TYPE_ONEOF: {
            CommandArgDefinitionExtra *arg_def_extra = &arg_def->extra;
            
            if (arg_def_extra->type != EXTRA_ARG_ARR) {
                PANIC("ONEOF arg type must have an arg array in 'extra'");
            }

            CommandArgDefinitionArray *arg_defs_arr = &arg_def_extra->value.arg_defs_arr;

            for (size_t i = 0; i < arg_defs_arr->arg_defs_len; i += 1) {
                CommandArgDefinition *inner_arg_def = &arg_defs_arr->arg_defs[i];

                CommandArgParseResult result = parse_command_argument(
                    arena,
                    inner_arg_def,
                    iaa,
                    output_command_arg
                );

                
                if (result == CMD_ARGS_PARSE_SUCCESS) {
                    // If the one of definition is optional then wrap the matched value with option
                    if (option) {
                        option->value = (*output_command_arg)->value;
                        option->is_present = true;

                        (*output_command_arg)->value = option;
                    }
                    

                    return result;
                }
            }

            // Argument is optional
            if (option != NULL) {
                *value = NULL;
                option->is_present = false;

                return CMD_ARGS_PARSE_SUCCESS;
            }

            return CMD_ARGS_TYPE_MISMATCH;
        }
        case ARG_TYPE_PURE_TOKEN: {
            static bool TRUE = true;
            *value = &TRUE;

            if (option) {
                option->is_present = true;
            }

            return CMD_ARGS_PARSE_SUCCESS;
        }
    }

    UNREACHABLE();
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

            for (size_t i = 0; i < iaa->args_len; i += 1) {
                iaa->resolved_args[i] = shrinked_iaa->resolved_args[i];
            }

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

    
    return CMD_ARGS_TOKEN_MISMATCH;
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
