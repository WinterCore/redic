#include <string.h>

#include "./cli.h"
#include "aids.h"
#include "arena.h"


/**
 * Parse program CLI args
 *
 * TODO: Test this properly and make sure it works with multiple opts
 */
bool cli_parse_opts(
    Arena *a,
    const size_t opt_defs_len,
    const CLIOptionDefinition opt_defs[opt_defs_len],
    const size_t iargs_len,
    char **iargs,
    CLIOption opts[opt_defs_len]
) {
    size_t opt_idx = 0;

    for (size_t i = 0; i < opt_defs_len; i += 1) {
        const CLIOptionDefinition *def = &opt_defs[i];
        CLIOption *cli_option = &opts[i];

        cli_option->is_optional = def->is_optional;

        int matched_idx = -1;

        for (size_t j = opt_idx; j < iargs_len; j += 1) {
            char *arg = iargs[j];

            if (
                // Long notation
                (strncmp(arg, "--", 2) == 0 && strcmp(def->name, &arg[2]) == 0)

                ||
                // Short notation
                (strncmp(arg, "-", 1) == 0 && strcmp(def->name, &arg[1]) == 0)
            ) {
                matched_idx = j;
                break;
            }
        }

        void **value = &cli_option->value;

        Option *option = NULL;

        if (def->is_optional) {
            option = option_create(a, NULL);
            cli_option->value = option;
            value = &option->value;
        }

        if (matched_idx == -1 && def->is_optional) {
            if (def->is_optional) {
                continue;
            } else {
                return false;
            }
        }

        // The number of args we've consumed so far (1 cuz we're including the --<key>)
        size_t consumed_args = 1;

        // At this point if the argument is optional then we have to have a value
        // which is going to be set later on
        if (option) {
            option->is_present = true;
        }

        // Parse
        
        switch (def->type) {
            case CLI_BOOL: {
                bool TRUE = true;
                *value = &TRUE;

                break;
            }
            case CLI_INTEGER: {
                if (iargs_len <= (size_t) matched_idx + 1) {
                    return false;
                }

                char *raw_value = iargs[matched_idx + 1];
                consumed_args += 1;

                long *integer = arena_alloc(a, sizeof(long));
                bool success = parse_long(raw_value, integer);
                
                if (! success) {
                    return false;
                }
                
                *value = integer;
                
                break;
            }
            case CLI_FLOAT: {
                UNIMPLEMENTED("CLI FLOAT OPT PARSING %s", "");
            }
            case CLI_STRING: {
                UNIMPLEMENTED("CLI FLOAT OPT STRING %s", "");
            }
            default: {
                UNIMPLEMENTED("UNKNOWN CLI ARG TYPE %d", def->type);
            }
        }
        
        // Move the args we just consumed to the beginning
        for (size_t j = matched_idx; j < consumed_args; j += 1, opt_idx += 1) {
            iargs[opt_idx] = iargs[j];
        }
    }

    return true;
}

