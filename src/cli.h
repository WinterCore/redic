#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

#include "./aids.h"

enum CLIOptionType {
    CLI_BOOL,
    CLI_STRING,
    CLI_INTEGER,
    CLI_FLOAT,
};

typedef struct CLIOptionDefinition {
    char *name;
    char *shorthand;
    enum CLIOptionType type;

    bool is_optional;
} CLIOptionDefinition;

typedef struct CLIOption {
    bool is_optional;
    void *value;
} CLIOption;

bool cli_parse_opts(
    Arena *a,
    const size_t opt_defs_len,
    const CLIOptionDefinition opt_defs[opt_defs_len],
    const size_t iargs_len,
    char **iargs,
    CLIOption opts[opt_defs_len]
);

#endif
