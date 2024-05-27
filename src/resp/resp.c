#include <stddef.h>
#include <string.h>

#include "./resp.h"


typedef struct RESPParser {
    char *input;
    size_t pos;
} RESPParser;

static RESPParseResult parse_simple_string(RESPParser *parser, RESPSimpleString *simple_string) {
    RESPParseResult result = { .code = RESP_PARSE_SUCCESS, .pos = parser->pos };

    // TODO: make sure that the input starts with the simple string marker
    size_t pos = parser->pos + 1;

    char *str_ptr = &parser->input[pos];
    DEBUG_PRINT("-hi %c", *str_ptr);
    size_t str_len = 0;

    while (*str_ptr != '\r' && *str_ptr != '\n') {
        str_len += 1;
        str_ptr += 1;
        DEBUG_PRINT("-hi %c", *str_ptr);
    }

    char *value = malloc(str_len * sizeof(char) + 1);

    if (value == NULL) {
        result.code = RESP_PARSE_MEMORY_ALLOC_FAILED;

        return result;
    }
    
    memcpy(value, str_ptr, str_len);

    value[str_len * sizeof(char)] = '\0';

    simple_string->string = value;
    

    pos += str_len;

    // TODO: Validate that input actually contains \r\n right after the value
    pos += 2;

    parser->pos = pos;

    return result;
}

static RESPParseResult parse_integer(RESPParser *parser, RESPInteger *integer) {
    RESPParseResult result = { .code = RESP_PARSE_SUCCESS, .pos = parser->pos };

    char *ptr = &parser->input[parser->pos];

    char **end_ptr = NULL;
    int64_t value = strtoll(&parser->input[parser->pos], end_ptr, 10);

    integer->value = value;
    parser->pos += (ptr - *end_ptr) + 2;

    return result;
}

static RESPParseResult resp_parse_value(RESPParser *parser, RESPValue *value) {
    DEBUG_PRINT("wotwot%c", *parser->input)

    switch (*parser->input) {
        case RESP_INTEGER: {
            RESPInteger *integer = malloc(sizeof(RESPInteger));

            if (integer == NULL) {
                return (RESPParseResult) { .code = RESP_PARSE_MEMORY_ALLOC_FAILED, .pos = parser->pos };
            }

            RESPParseResult result =  parse_integer(parser, integer);

            if (result.code == RESP_PARSE_SUCCESS) {
                value->value = integer;
                value->kind = RESP_INTEGER;
            }

            return result;
        }

        case RESP_SIMPLE_STRING: {
            RESPSimpleString *simple_string = malloc(sizeof(RESPSimpleString));

            if (simple_string == NULL) {
                return (RESPParseResult) { .code = RESP_PARSE_MEMORY_ALLOC_FAILED, .pos = parser->pos };
            }
            
            RESPParseResult result = parse_simple_string(parser, simple_string);

            if (result.code == RESP_PARSE_SUCCESS) {
                value->value = simple_string;
                value->kind = RESP_SIMPLE_STRING;
            }

            return result;
            
        }

        case RESP_NULL: {
            RESPNull *null = malloc(sizeof(RESPNull));

            if (null == NULL) {
                return (RESPParseResult) { .code = RESP_PARSE_MEMORY_ALLOC_FAILED, .pos = parser->pos };
            }

            parser->pos += 3; // _\r\n

            value->value = null;
            value->kind = RESP_NULL;

            return (RESPParseResult) { .code = RESP_PARSE_SUCCESS, .pos = parser->pos };
        }
    }
    

    // Unknown data type
    return (RESPParseResult) { .code = RESP_PARSE_UNKNOWN_DATA_TYPE_MARKER, .pos = parser->pos };
}

RESPParseResult resp_parse_input(char *input, RESPValue *value) {
    RESPParser parser = {
        .input = input,
        .pos = 0,
    };
    printf("Inside parse input");
    fflush(stdout);

    DEBUG_PRINT("HELLO %s", "");

    return resp_parse_value(&parser, value);
}
