#include <stddef.h>
#include <string.h>

#include "./resp.h"

#define CONSUME_TOKEN(TOKEN, STR_PTR, POS) \
    if (*STR_PTR != TOKEN) { \
        return (RESPParseResult) { .code = RESP_PARSE_UNEXPECTED_TOKEN, .pos = *POS }; \
    } \
    STR_PTR += 1; \
    *POS += 1    

typedef struct RESPParser {
    char *input;
    size_t pos;
} RESPParser;

static RESPParseResult resp_parse_value(RESPParser *parser, RESPValue *value);
void destroy_value(RESPValue *value);

static RESPParseResult parse_simple_string(RESPParser *parser, RESPSimpleString *simple_string) {
    RESPParseResult result = { .code = RESP_PARSE_SUCCESS, .pos = parser->pos };

    size_t pos = parser->pos;
    char *ptr = &parser->input[pos];

    CONSUME_TOKEN(RESP_SIMPLE_STRING, ptr, &pos);

    size_t str_len = 0;

    while (*ptr != '\r' && *ptr != '\n') {
        str_len += 1;
        ptr += 1;
    }

    char *value = malloc(str_len * sizeof(char) + 1);

    if (value == NULL) {
        result.code = RESP_PARSE_MEMORY_ALLOC_FAILED;

        return result;
    }
    
    memcpy(value, &parser->input[pos], str_len);

    value[str_len * sizeof(char)] = '\0';
    simple_string->string = value;

    pos += str_len;

    CONSUME_TOKEN('\r', ptr, &pos);
    CONSUME_TOKEN('\n', ptr, &pos);

    parser->pos = pos;

    return result;
}

void destroy_simple_string(RESPSimpleString *simple_string) {
    free(simple_string->string);
}

static RESPParseResult parse_integer(RESPParser *parser, RESPInteger *integer) {
    RESPParseResult result = { .code = RESP_PARSE_SUCCESS, .pos = parser->pos };

    size_t pos = parser->pos;
    char *ptr = &parser->input[parser->pos];

    CONSUME_TOKEN(RESP_INTEGER, ptr, &pos);

    char *end_ptr = NULL;
    int64_t value = strtoll(ptr, &end_ptr, 10);

    integer->value = value;
    pos += (end_ptr - ptr);
    ptr += (end_ptr - ptr);

    CONSUME_TOKEN('\r', ptr, &pos);
    CONSUME_TOKEN('\n', ptr, &pos);

    parser->pos = pos;

    return result;
}

void destroy_integer(RESPInteger *integer) {
    // noop
}

static RESPParseResult parse_null(RESPParser *parser, RESPNull *null) {
    RESPParseResult result = { .code = RESP_PARSE_SUCCESS, .pos = parser->pos };
    char *ptr = &parser->input[parser->pos];
    size_t pos = parser->pos;

    CONSUME_TOKEN('_', ptr, &pos);
    CONSUME_TOKEN('\r', ptr, &pos);
    CONSUME_TOKEN('\n', ptr, &pos);

    parser->pos = pos;

    return result;
}

void destroy_null(RESPNull *null) {
    // noop
}

static RESPParseResult parse_array(RESPParser *parser, RESPArray *array) {
    RESPParseResult result = { .code = RESP_PARSE_SUCCESS, .pos = parser->pos };

    char *ptr = &parser->input[parser->pos];
    size_t pos = parser->pos;

    CONSUME_TOKEN(RESP_ARRAY, ptr, &pos);

    char *end_ptr = NULL;
    size_t length = strtoull(ptr, &end_ptr, 10);

    pos += (end_ptr - ptr);
    ptr += (end_ptr - ptr);

    CONSUME_TOKEN('\r', ptr, &pos);
    CONSUME_TOKEN('\n', ptr, &pos);

    parser->pos = pos;

    Hector *hector = hector_create(sizeof(RESPValue), length);

    while (length > 0) {
        RESPValue *array_elem = malloc(sizeof(RESPValue));

        RESPParseResult result = resp_parse_value(parser, array_elem);

        if (result.code != RESP_PARSE_SUCCESS) {
            return result;
        }

        hector_push(hector, array_elem);

        length -= 1;
    }

    array->array = hector;

    return result;
}

void destroy_array(RESPArray *array) {
    Hector *hector = array->array;

    // Destroy all the values recursively
    for (size_t i = 0; i < hector->length; i += 1) {
        RESPValue *value = hector_get(hector, i);
        destroy_value(value);
    }

    // Destroy the vector
    hector_destroy(hector);
}


void destroy_value(RESPValue *value) {
    switch (value->kind) {
        case RESP_INTEGER: {
            destroy_integer(value->value);
            break;
        }

        case RESP_SIMPLE_STRING: {
            destroy_simple_string(value->value);
            break;
        }

        case RESP_NULL: {
            destroy_null(value->value);
            break;
        }

        case RESP_ARRAY: {
            destroy_array(value->value);
            break;
        }

        default: {
            UNIMPLEMENTED("destructor for %d", value->kind);
        }
    }
}

static RESPParseResult resp_parse_value(RESPParser *parser, RESPValue *value) {
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

            RESPParseResult result = parse_null(parser, null);

            if (result.code == RESP_PARSE_SUCCESS) {
                value->value = null;
                value->kind = RESP_NULL;
            }

            return result;
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

    DEBUG_PRINT("HELLO %s", "");

    return resp_parse_value(&parser, value);
}
