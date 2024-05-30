#include <stdio.h>

#include "../aids.h"
#include "./debug.h"

#define PRINT_INDENTED_FMT(LEVEL, fmt, ...) \
    for (size_t i = 0; i < LEVEL; i += 1) { \
        printf("  "); \
    } \
    printf(fmt, __VA_ARGS__);

#define PRINT_INDENTED(LEVEL, fmt) \
    PRINT_INDENTED_FMT(LEVEL, fmt "%s", "");

static void print_simple_string(size_t indent_level, RESPSimpleString *simple_string) {
}

static void print_bulk_string(size_t indent_level, RESPBulkString *bulk_string) {
}

static void print_array(size_t indent_level, RESPArray *array) {
}

static void print_null(size_t indent_level, RESPNull *null) {
}

static void print_integer(size_t indent_level, RESPInteger *integer) {
    PRINT_INDENTED(indent_level, "RESPInteger {");
    PRINT_INDENTED_FMT(indent_level + 1, "value = %lld", integer->value);
    PRINT_INDENTED(indent_level, "}");
}

static void print_value_helper(size_t indent_level, RESPValue *value) {
    switch (value->kind) {
        case RESP_SIMPLE_STRING: {
            print_simple_string(indent_level, value->value);
            break;
        }

        case RESP_BULK_STRING: {
            print_bulk_string(indent_level, value->value);
            break;
        }

        case RESP_ARRAY: {
            print_array(indent_level, value->value);
            break;
        }

        case RESP_NULL: {
            print_null(indent_level, value->value);
            break;
        }
                        
        case RESP_INTEGER: {
            print_integer(indent_level, value->value);
            break;
        }

        default: {
            UNIMPLEMENTED("debug_print for %d", value->kind);
            break;
        }
    }
}

void print_value(RESPValue *value) {
    printf("RESPValue {");
    print_value_helper(1, value);
    printf("}");
}
