#include <string.h>
#include <inttypes.h>

#include "./resp.h"

typedef struct RESPSerializer {
    size_t pos;
    char *buffer;
} RESPSerializer;

void serialize_value_helper(RESPSerializer *serializer, RESPValue *value);

static void serialize_null(RESPSerializer *serializer, RESPNull *null) {
    UNUSED(null);
    char *ptr = &serializer->buffer[serializer->pos];

    ptr[0] = RESP_NULL;
    ptr[1] = '\r';
    ptr[2] = '\n';

    serializer->pos += 1 + 2;
}

static void serialize_simple_string(RESPSerializer *serializer, RESPSimpleString *simple_string) {
    char *ptr = &serializer->buffer[serializer->pos];

    size_t len = strlen(simple_string->string);

    ptr[0] = RESP_SIMPLE_STRING;

    memcpy(&ptr[1], simple_string->string, len);

    ptr += len + 1;

    ptr[0] = '\r';
    ptr[1] = '\n';
    
    serializer->pos += 1 + len + 2;
}

static void serialize_bulk_string(RESPSerializer *serializer, RESPBulkString *bulk_string) {
    char *ptr = &serializer->buffer[serializer->pos];
    
    ptr[0] = RESP_BULK_STRING;
    ptr += 1;

    int len_char_count = sprintf(ptr, "%" PRIu64, bulk_string->length);
    ptr += len_char_count;

    ptr[0] = '\r';
    ptr[1] = '\n';
    ptr += 2;

    memcpy(ptr, bulk_string->data, bulk_string->length);
    ptr += bulk_string->length;

    ptr[0] = '\r';
    ptr[1] = '\n';

    serializer->pos += 1 + len_char_count + 2 + bulk_string->length + 2;
}

static void serialize_integer(RESPSerializer *serializer, RESPInteger *integer) {
    char *ptr = &serializer->buffer[serializer->pos];

    ptr[0] = RESP_INTEGER;
    ptr += 1;

    int value_char_count = sprintf(ptr, "%" PRId64, integer->value);
    ptr += value_char_count;

    ptr[0] = '\r';
    ptr[1] = '\n';


    serializer->pos += 1 + value_char_count + 2;
}

static void serialize_array(RESPSerializer *serializer, RESPArray *array) {
    char *ptr = &serializer->buffer[serializer->pos];
    
    ptr[0] = RESP_BULK_STRING;
    ptr += 1;

    int len_char_count = sprintf(ptr, "%zu", array->array->length);
    ptr += len_char_count;

    ptr[0] = '\r';
    ptr[1] = '\n';
    ptr += 2;

    serializer->pos += 1 + len_char_count + 2;

    if (array->array->length == 0) {
        return;
    }

    for (size_t i = 0; i < array->array->length; i += 1) {
        RESPValue *item = hector_get(array->array, i);

        serialize_value_helper(serializer, item);
    }
}

void serialize_value_helper(RESPSerializer *serializer, RESPValue *value) {
    switch (value->kind) {
        case RESP_INTEGER: {
            serialize_integer(serializer, value->value);
            break;
        }

        case RESP_SIMPLE_STRING: {
            serialize_simple_string(serializer, value->value);
            break;
        }

        case RESP_BULK_STRING: {
            serialize_bulk_string(serializer, value->value);
            break;
        }

        case RESP_NULL: {
            serialize_null(serializer, value->value);
            break;
        }

        case RESP_ARRAY: {
            serialize_array(serializer, value->value);
            break;
        }

        default: {
            UNIMPLEMENTED("serializer for %d", value->kind);
        }
    }
}

size_t resp_serialize_value(char *buffer, RESPValue *value) {
    RESPSerializer serializer = {
        .pos = 0,
        .buffer = buffer,
    };

    serialize_value_helper(&serializer, value);

    serializer.buffer[serializer.pos] = '\0';

    return serializer.pos;
}
