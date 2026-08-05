#include <stdint.h>
#include <stdio.h>

#include "potty_serializer.h"
#include "../aids.h"

// @deadcode for now, not sure if it'll be needed later
size_t get_mutation_size(SepticTankMutation *mutation) {
    switch (mutation->type) {
        case SEPTIC_TANK_GET:
        case SEPTIC_TANK_TTL:
        case SEPTIC_TANK_EXISTS:
        case SEPTIC_TANK_INCRBY:
            UNREACHABLE();
            break;

        case SEPTIC_TANK_DEL:
            return (
                1                                   + // Type
                sizeof(mutation->del.key->len)      + // Key Length
                mutation->del.key->len                // Key
            );

        case SEPTIC_TANK_SET:
            return (
                1                                   + // Type
                sizeof(mutation->set.expiration.ts) + // Expiration

                sizeof(mutation->set.key->len)      + // Key Length
                mutation->set.key->len              + // Key

                sizeof(mutation->set.value->len)    + // Key Length
                mutation->set.value->len              // Value
            );
            break;

        case SEPTIC_TANK_EXPIRE:
            return (
                1                                     + // Type
                sizeof(mutation->expire.key->len)     + // Key Length
                mutation->expire.key->len             + // Key
                sizeof(mutation->expire.expires_at)     // Expiration
            );
            break;
    }
}

static void fwrite_string(FILE *file, DataString *str) {
    uint64_t len = str->len;
    uint64_t items = fwrite(&len, sizeof(len), 1, file);

    if (items != 1) {
        PANIC("fwrite_string failed");
    }

    items = fwrite(str->str, len, 1, file);

    if (items != 1) {
        PANIC("fwrite_string failed");
    }
}

static void fwrite_int64(FILE *file, int64_t *value) {
    uint64_t items = fwrite(value, sizeof(int64_t), 1, file);

    if (items != 1) {
        PANIC("fwrite_int64 failed");
    }
}

static void fwrite_byte(FILE *file, uint8_t *value) {
    uint64_t items = fwrite(value, sizeof(uint8_t), 1, file);

    if (items != 1) {
        PANIC("fwrite_byte failed");
    }
}

void potty_serialize_mutation(FILE *file, SepticTankMutation *mutation) {
    uint8_t type = mutation->type;
    fwrite_byte(file, &type); // Type

    switch (mutation->type) {
        case SEPTIC_TANK_SET: {
            // Expiration
            fwrite_int64(file, &mutation->set.expiration.ts);

            // Key
            fwrite_string(file, mutation->set.key);

            // Value
            fwrite_string(file, mutation->set.value);

            break;
        }
        case SEPTIC_TANK_DEL: {
            // Key
            fwrite_string(file, mutation->del.key);

            break;
        }
        case SEPTIC_TANK_EXPIRE: {
            // Key
            fwrite_string(file, mutation->expire.key);

            // Expiration
            fwrite_int64(file, &mutation->expire.expires_at);

            break;
        }
        case SEPTIC_TANK_GET:
        case SEPTIC_TANK_TTL:
        case SEPTIC_TANK_EXISTS:
        case SEPTIC_TANK_INCRBY:
            UNREACHABLE();
    }
}
