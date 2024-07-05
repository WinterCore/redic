#ifndef RESP_H
#define RESP_H

#include <stdint.h>

#include "../aids.h"
#include "../arena.h"

typedef enum RESPParseResultCode {
    RESP_PARSE_SUCCESS,
    RESP_PARSE_UNKNOWN_DATA_TYPE_MARKER,
    RESP_PARSE_UNEXPECTED_TOKEN,
    RESP_PARSE_EMPTY_INPUT,
    RESP_PARSE_MEMORY_ALLOC_FAILED,
} RESPParseResultCode;

typedef struct RESPParseResult {
    RESPParseResultCode code;
    size_t pos;
} RESPParseResult;

typedef enum RESPValueKind {
    RESP_SIMPLE_STRING = '+',
    RESP_BULK_STRING = '$',
    RESP_ARRAY = '*',
    RESP_NULL = '_',
    RESP_INTEGER = ':',
    RESP_SIMPLE_ERROR = '-',
} RESPValueKind;

typedef struct RESPValue {
    RESPValueKind kind;
    void *value;
} RESPValue;

typedef struct RESPSimpleString {
    char *string;
} RESPSimpleString;

typedef struct RESPBulkString {
    uint64_t length;
    char *data;
} RESPBulkString;

typedef struct RESPArray {
    // Hector<RESPValue> Element type is RESPValue
    Hector *array;
} RESPArray;

typedef struct RESPInteger {
    int64_t value;
} RESPInteger;

typedef struct RESPNull {
} RESPNull;

typedef struct RESPSimpleError {
    char *message;
} RESPSimpleError;

RESPParseResult resp_parse_input(Arena *arena, char *input, RESPValue *value);

void resp_print_parse_result(RESPParseResult *result);

void resp_print_value(RESPValue *value);

// TODO: Maybe take a buffer length as well so we can prevent overflowing
size_t resp_serialize_value(char *buffer, RESPValue *value);


/**
 * Value Constructors
 */

RESPValue resp_create_simple_error_value(Arena *arena, char *value);
RESPValue resp_create_simple_string_value(Arena *arena, char *value);
RESPValue resp_create_bulk_string_value(Arena *arena, size_t len, char *string);
RESPValue resp_create_null_value(Arena *arena);

#endif
