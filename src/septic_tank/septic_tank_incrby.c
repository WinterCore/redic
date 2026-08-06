#include <stdint.h>

#include "septic_tank_operation.h"
#include "septic_tank.h"

SepticTankResult *septic_tank_resolve_incrby(SepticTank *tank, Arena *result_arena, SepticTankIncrByOperation *op) {
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
        // Treated as absent — the SET mutation below overwrites it
    }

    if (entry != NULL) {
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

    // An existing key keeps its expiry; a missing or expired one starts fresh
    OptionTime expires_at = entry != NULL
        ? entry->expires_at
        : (OptionTime) { .is_present = false };

    result->incrby_result.value = value;

    // INCRBY isn't a persisted mutation type — it resolves into the SET it performs
    result->resolved_mutation = arena_alloc(result_arena, sizeof(SepticTankMutation));
    result->resolved_mutation->type = SEPTIC_TANK_SET;
    result->resolved_mutation->set = (SepticTankSetMutation) {
        .key = op->key,
        .value = string_value,
        .expires_at = expires_at.is_present
            ? expires_at.value
            : ST_MUTATION_NO_EXPIRY,
    };

    return result;
}
