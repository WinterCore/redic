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

static RESPParseResult resp_parse_value(Arena *arena, RESPParser *parser, RESPValue *value);

static RESPParseResult parse_simple_string(Arena *arena, RESPParser *parser, RESPSimpleString *simple_string) {
    RESPParseResult result = { .code = RESP_PARSE_SUCCESS, .pos = parser->pos };

    size_t pos = parser->pos;
    char *ptr = &parser->input[pos];

    CONSUME_TOKEN(RESP_SIMPLE_STRING, ptr, &pos);

    size_t str_len = 0;

    while (*ptr != '\r' && *ptr != '\n') {
        str_len += 1;
        ptr += 1;
    }

    char *value = arena_alloc(arena, str_len * sizeof(char) + 1);

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

static RESPParseResult parse_simple_error(Arena *arena, RESPParser *parser, RESPSimpleError *simple_error) {
    RESPParseResult result = { .code = RESP_PARSE_SUCCESS, .pos = parser->pos };

    size_t pos = parser->pos;
    char *ptr = &parser->input[pos];

    CONSUME_TOKEN(RESP_SIMPLE_ERROR, ptr, &pos);

    size_t str_len = 0;

    while (*ptr != '\r' && *ptr != '\n') {
        str_len += 1;
        ptr += 1;
    }

    char *value = arena_alloc(arena, str_len * sizeof(char) + 1);

    if (value == NULL) {
        result.code = RESP_PARSE_MEMORY_ALLOC_FAILED;

        return result;
    }
    
    memcpy(value, &parser->input[pos], str_len);

    value[str_len * sizeof(char)] = '\0';
    simple_error->message = value;

    pos += str_len;

    CONSUME_TOKEN('\r', ptr, &pos);
    CONSUME_TOKEN('\n', ptr, &pos);

    parser->pos = pos;

    return result;
}

static RESPParseResult parse_bulk_string(Arena *arena, RESPParser *parser, RESPBulkString *bulk_string) {
    RESPParseResult result = { .code = RESP_PARSE_SUCCESS, .pos = parser->pos };

    char *ptr = &parser->input[parser->pos];
    size_t pos = parser->pos;

    CONSUME_TOKEN(RESP_BULK_STRING, ptr, &pos);

    char *end_ptr = NULL;
    size_t length = strtoull(ptr, &end_ptr, 10);

    pos += (end_ptr - ptr);
    ptr += (end_ptr - ptr);

    CONSUME_TOKEN('\r', ptr, &pos);
    CONSUME_TOKEN('\n', ptr, &pos);

    char *value = arena_alloc(arena, length * sizeof(char) + 1);

    if (value == NULL) {
        result.code = RESP_PARSE_MEMORY_ALLOC_FAILED;

        return result;
    }

    memcpy(value, &parser->input[pos], length);

    bulk_string->length = length;
    bulk_string->data = value;

    pos += length;
    ptr += length;
    
    CONSUME_TOKEN('\r', ptr, &pos);
    CONSUME_TOKEN('\n', ptr, &pos);

    parser->pos = pos;

    return result;
}

static RESPParseResult parse_integer(Arena *arena, RESPParser *parser, RESPInteger *integer) {
    UNUSED(arena);
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
    DEBUG_PRINT("within %zu", pos);

    parser->pos = pos;

    return result;
}

static RESPParseResult parse_null(Arena *arena, RESPParser *parser, RESPNull *null) {
    UNUSED(arena);
    UNUSED(null);
    RESPParseResult result = { .code = RESP_PARSE_SUCCESS, .pos = parser->pos };
    char *ptr = &parser->input[parser->pos];
    size_t pos = parser->pos;

    CONSUME_TOKEN('_', ptr, &pos);
    CONSUME_TOKEN('\r', ptr, &pos);
    CONSUME_TOKEN('\n', ptr, &pos);

    parser->pos = pos;

    return result;
}

static RESPParseResult parse_array(Arena *arena, RESPParser *parser, RESPArray *array) {
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

    Hector *hector = hector_create(arena, sizeof(RESPValue), length);

    while (length > 0) {
        RESPValue *array_elem = arena_alloc(arena, sizeof(RESPValue));

        RESPParseResult result = resp_parse_value(arena, parser, array_elem);

        if (result.code != RESP_PARSE_SUCCESS) {
            return result;
        }

        hector_push(hector, array_elem);

        length -= 1;
    }

    array->array = hector;

    return result;
}

static RESPParseResult resp_parse_value(Arena *arena, RESPParser *parser, RESPValue *value) {
    #define PARSE_VALUE(KIND, STRUCT, PARSE_FN) \
        STRUCT *kind_value = arena_alloc(arena, sizeof(STRUCT)); \
        if (kind_value == NULL) { \
            return (RESPParseResult) { .code = RESP_PARSE_MEMORY_ALLOC_FAILED, .pos = parser->pos }; \
        } \
        RESPParseResult result =  PARSE_FN(arena, parser, kind_value); \
        if (result.code == RESP_PARSE_SUCCESS) { \
            value->value = kind_value; \
            value->kind = KIND; \
        } \
        return result
        

    switch (parser->input[parser->pos]) {
        case RESP_INTEGER: {
            PARSE_VALUE(RESP_INTEGER, RESPInteger, parse_integer);
        }

        case RESP_SIMPLE_STRING: {
            PARSE_VALUE(RESP_SIMPLE_STRING, RESPSimpleString, parse_simple_string);
        }

        case RESP_SIMPLE_ERROR: {
            PARSE_VALUE(RESP_SIMPLE_ERROR, RESPSimpleError, parse_simple_error);
        }

        case RESP_BULK_STRING: {
            PARSE_VALUE(RESP_BULK_STRING, RESPBulkString, parse_bulk_string);
        }

        case RESP_NULL: {
            PARSE_VALUE(RESP_NULL, RESPNull, parse_null);
        }

        case RESP_ARRAY: {
            PARSE_VALUE(RESP_ARRAY, RESPArray, parse_array);
        }
    }
    

    // Unknown data type
    return (RESPParseResult) { .code = RESP_PARSE_UNKNOWN_DATA_TYPE_MARKER, .pos = parser->pos };
}

RESPParseResult resp_parse_input(Arena *arena, char *input, RESPValue *value) {
    RESPParser parser = {
        .input = input,
        .pos = 0,
    };

    return resp_parse_value(arena, &parser, value);
}
