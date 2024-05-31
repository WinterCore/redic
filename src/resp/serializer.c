#include "./resp.h"

typedef struct RESPSerializer {
    size_t pos;
    char *buffer;
} RESPSerializer;

static void serialize_null(RESPSerializer *serializer, RESPNull *null) {
    char *ptr = &serializer->buffer[serializer->pos];

    ptr[0] = '_';
    ptr[1] = '\r';
    ptr[2] = '\n';

    serializer->pos += 3;
}

void serialize_value_helper(RESPSerializer *serializer, RESPValue *value) {
    switch (value->kind) {
        case RESP_NULL: {
            serialize_null(serializer, value->value);

            break;
        }

        default: {
            UNIMPLEMENTED("serializer for %d", value->kind);
        }
    }
}

void resp_serialize_value(char *buffer, RESPValue *value) {
    RESPSerializer serializer = {
        .pos = 0,
        .buffer = buffer,
    };

    serialize_value_helper(&serializer, value);

    serializer.buffer[serializer.pos] = '\0';
}
