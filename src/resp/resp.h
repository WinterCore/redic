#ifndef RESP_H
#define RESP_H

#include <stdint.h>

#include "../aids.h"

enum RESPParseResultCode {
    RESP_PARSE_SUCCESS,
    RESP_PARSE_UNKNOWN_DATA_TYPE_MARKER,
    RESP_PARSE_UNEXPECTED_TOKEN,
    RESP_PARSE_EMPTY_INPUT,
    RESP_PARSE_MEMORY_ALLOC_FAILED,
};

typedef struct RESPParseResult {
    uint8_t code;
    size_t pos;
} RESPParseResult;

enum RESPValueKind {
    RESP_SIMPLE_STRING = '+',
    RESP_ARRAY = '*',
    RESP_NULL = '_',
    RESP_INTEGER = ':',
};

typedef struct RESPValue {
    enum RESPValueKind kind;
    void *value;
} RESPValue;

typedef struct RESPSimpleString {
    char *string;
} RESPSimpleString;

typedef struct RESPArray {
    // Element type is RESPValue
    Hector array;
} RESPArray;

typedef struct RESPInteger {
    int64_t value;
} RESPInteger;

typedef struct RESPNull {
} RESPNull;

RESPParseResult resp_parse_input(char *input, RESPValue *value);

#endif
