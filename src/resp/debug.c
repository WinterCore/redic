#include <stdio.h>
#include <inttypes.h>

#include "./resp.h"
#include "../aids.h"

#define PRINT_INDENTED_FMT(LEVEL, fmt, ...) \
    for (size_t i = 0; i < LEVEL; i += 1) { \
        printf("  "); \
    } \
    printf(fmt, __VA_ARGS__); \
    fflush(stdout);

#define PRINT_INDENTED(LEVEL, fmt) \
    PRINT_INDENTED_FMT(LEVEL, fmt "%s", "");

static void print_value_helper(size_t indent_level, RESPValue *value);

static void print_simple_string(size_t indent_level, RESPSimpleString *simple_string) {
    PRINT_INDENTED(indent_level, "RESPBulkString {\n");

    PRINT_INDENTED_FMT(indent_level + 1, "%s\n", simple_string->string);

    PRINT_INDENTED(indent_level, "}\n");
}

static void print_bulk_string(size_t indent_level, RESPBulkString *bulk_string) {
    PRINT_INDENTED(indent_level, "RESPBulkString {\n");

    PRINT_INDENTED(indent_level + 1, "data = [");
    for (size_t i = 0; i < bulk_string->length; i += 1) {
        printf("%s0x%X", i == 0 ? "" : ", ",  bulk_string->data[i]);
    }
    printf("]\n");

    PRINT_INDENTED(indent_level, "}\n");
}

static void print_array(size_t indent_level, RESPArray *array) {
    PRINT_INDENTED_FMT(indent_level, "RESPArray(length = %zu) {\n", array->array->length);

    for (size_t i = 0; i < array->array->length; i += 1) {
        RESPValue *value = hector_get(array->array, i);

        print_value_helper(indent_level + 1, value);
    }

    PRINT_INDENTED(indent_level, "}\n");
}

static void print_null(size_t indent_level, RESPNull *null) {
    PRINT_INDENTED(indent_level, "RESPNull {}\n");
}

static void print_integer(size_t indent_level, RESPInteger *integer) {
    PRINT_INDENTED(indent_level, "RESPInteger {\n");
    PRINT_INDENTED_FMT(indent_level + 1, "value = %" PRId64, integer->value);
    PRINT_INDENTED(indent_level, "}\n");
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

void resp_print_parse_result(RESPParseResult *result) {
    switch (result->code) {
    case RESP_PARSE_SUCCESS: {
        PRINT_INDENTED(0, "RESPParseResult(SUCCESS)\n");
        break;
    }

    case RESP_PARSE_UNKNOWN_DATA_TYPE_MARKER: {
        PRINT_INDENTED_FMT(0, "RESPParseResult(UNKNOWN_DATA_TYPE_MARKER) { pos = %zu }\n", result->pos);
        break;
    }

    case RESP_PARSE_UNEXPECTED_TOKEN: {
        PRINT_INDENTED_FMT(0, "RESPParseResult(UNEXPECTED_TOKEN) { pos = %zu }\n", result->pos);
        break;
    }

    case RESP_PARSE_EMPTY_INPUT: {
        PRINT_INDENTED(0, "RESPParseResult(EMPTY_INPUT)\n");
        break;
    }

    case RESP_PARSE_MEMORY_ALLOC_FAILED:
        PRINT_INDENTED(0, "RESPParseResult(MEM_ALLOC_FAILED)\n");

        break;
    }
}

void resp_print_value(RESPValue *value) {
    PRINT_INDENTED(0, "RESPValue {\n")
    print_value_helper(1, value);
    PRINT_INDENTED(0, "}\n")
}
