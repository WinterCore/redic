#include "./resp.h"

#define CREATE_RESP_VALUE(KIND, VALUE) \
    ((RESPValue) { .kind = KIND, .value = VALUE })

RESPValue resp_create_simple_error_value(Arena *arena, char *value) {
    RESPSimpleError *simple_error = arena_alloc(arena, sizeof(RESPSimpleError));
    simple_error->message = value;

    return CREATE_RESP_VALUE(RESP_SIMPLE_ERROR, simple_error);
}

RESPValue resp_create_simple_string_value(Arena *arena, char *value) {
    RESPSimpleString *simple_string = arena_alloc(arena, sizeof(RESPSimpleString));
    simple_string->string = value;

    return CREATE_RESP_VALUE(RESP_SIMPLE_STRING, simple_string);
}

RESPValue resp_create_bulk_string_value(Arena *arena, size_t len, char *string) {
    RESPBulkString *bulk_string = arena_alloc(arena, sizeof(RESPBulkString));
    bulk_string->data = string;
    bulk_string->length = len;

    return CREATE_RESP_VALUE(RESP_BULK_STRING, bulk_string);
}

RESPValue resp_create_null_value(Arena *arena) {
    RESPBulkString *null = arena_alloc(arena, sizeof(RESPNull));

    return CREATE_RESP_VALUE(RESP_NULL, null);
}
