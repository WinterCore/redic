#include <stdint.h>
#include <string.h>

#include "septic_tank_operation.h"
#include "septic_tank.h"

SepticTankResult *septic_tank_incrby(SepticTank *tank, Arena *result_arena, SepticTankIncrByOperation *op) {
    SepticTankResult *result = arena_alloc(result_arena, sizeof(SepticTankResult));
    result->type = SEPTIC_TANK_INCRBY;
    result->resolved_mutation = NULL;
    result->error = NULL;
    result->success = true;
    result->incrby_result = (SepticTankIncrByResult) { .value = 0 };

    DataEntry *entry = NULL;
    
    hashmap_get(tank->data, op->key->str, op->key->len, (void **) &entry);

    int64_t value = 0;

    if (entry != NULL && data_is_expired(entry)) {
        entry = NULL;
        // Will be overwritten below
    }

    if (entry != NULL && !data_is_expired(entry)) {
        if (entry->type != DATA_STRING) {
            result->error = "WRONGTYPE Operation against a key holding the wrong kind of value";
            result->success = false;
            return result;
        }

        DataString *string_entry = data_unwrap_string(entry);

        bool parsed = parse_int64(string_entry->str, string_entry->len, &value);

        if (! parsed) {
            result->error = "ERR value is not an integer or out of range";
            result->success = false;
            return result;
        }
    }

    if ((op->delta > 0 && value > INT64_MAX - op->delta) ||
        (op->delta < 0 && value < INT64_MIN - op->delta)) {
        result->error = "Integer overflow";
        result->success = false;
        return result;
    }

    value += op->delta;

    DataString *string_value = data_string_from_int64(result_arena, value);

    OptionTime expires_at = entry != NULL
        ? entry->expires_at
        : (OptionTime) { .is_present = false };

    entry = data_create_string_entry(expires_at, string_value->len, string_value->str);
    char *stored_key = malloc(op->key->len);
    memcpy(stored_key, op->key->str, op->key->len);
    hashmap_put(tank->data, stored_key, op->key->len, entry);

    result->incrby_result.value = value;
    result->resolved_mutation = arena_alloc(result_arena, sizeof(SepticTankMutation));
    result->resolved_mutation->type = SEPTIC_TANK_SET;
    result->resolved_mutation->set = (SepticTankSetOperation) {
        .expiration = entry->expires_at.is_present
            ? (SepticTankExpiration) { .type = ST_EXPIRATION_UNIX_TS, .ts = entry->expires_at.value }
            : (SepticTankExpiration) { .type = ST_EXPIRATION_NO_EXPIRE },
        .key = op->key,
        .value = string_value,
    };

    return result;
}
