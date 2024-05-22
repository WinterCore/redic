#ifndef RESP_H
#define RESP_H

#include "../aids.h"

enum RESPValueKind {
    STRING = '+',
    ARRAY = '*',
};

typedef struct RESPValue {
    RESPValueKind kind;
    void *value;
} RESPValue;

typedef struct RESPSimpleString {
    char *string;
} RESPSimpleString;

typedef struct RESPArray {
    // Element type is RESPValue
    Hector array;
} RESPArray;



#endif
