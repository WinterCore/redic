#include <stdlib.h>
#include <string.h>

#include "septic_tank.h"
#include "septic_tank_operation.h"
#include "../data.h"

SepticTankResult *septic_tank_set(SepticTank *tank, Arena *result_arena, SepticTankSetOperation *op) {
    DataEntry *old_entry = NULL;
    
    SepticTankResult *result = arena_alloc(result_arena, sizeof(SepticTankResult));
    result->type = SEPTIC_TANK_SET;
    result->set_result = (SepticTankSetResult) { .old_value = NULL };
    result->resolved_mutation = NULL;
    result->error = NULL;
    result->success = true;

    hashmap_get(tank->data, op->key->str, op->key->len, (void **) &old_entry);
    bool old_entry_is_expired = old_entry != NULL && data_is_expired(old_entry);

    // Data type check
    if (old_entry && !old_entry_is_expired && old_entry->type != DATA_STRING) {
        result->success = false;
        result->error = "WRONGTYPE Operation against a key holding the wrong kind of value";
        return result;
    }

    // NX with an existing non-expired value
    if (op->nx && old_entry != NULL && !old_entry_is_expired) {
        if (op->get) {
            DataString *old_value = data_unwrap_string(old_entry);
            result->set_result.old_value = data_copy_string_arena(result_arena, old_value);
            return result;
        }

        return result;
    }

    // XX with no existing value
    if (op->xx && (old_entry == NULL || old_entry_is_expired)) {
        if (old_entry_is_expired) {
            hashmap_remove(tank->data, op->key->str, op->key->len);
        }

        return result;
    }

    // Remove expired entry
    if (old_entry != NULL && old_entry_is_expired) {
        hashmap_remove(tank->data, op->key->str, op->key->len);
        old_entry = NULL;
    }
    
    OptionTime expires_at;

    switch (op->expiration.type) {
        case ST_EXPIRATION_NO_EXPIRE:
            expires_at.is_present = false;
            break;
        case ST_EXPIRATION_UNIX_TS:
            expires_at.is_present = true;
            expires_at.value = op->expiration.ts;
            break;
        case ST_EXPIRATION_KEEP_OLD:
            if (old_entry != NULL) {
                expires_at = old_entry->expires_at;
            } else {
                expires_at.is_present = false;
            }
            break;
    }

    // Snapshot old value before hashmap_put auto-frees the old entry
    if (op->get && old_entry != NULL) {
        DataString *old_value = data_unwrap_string(old_entry);
        result->set_result.old_value = data_copy_string_arena(result_arena, old_value);
    }

    DataEntry *entry = data_create_string_entry(expires_at, op->value->len, op->value->str);
    char *stored_key = malloc(op->key->len);
    memcpy(stored_key, op->key->str, op->key->len);
    hashmap_put(tank->data, stored_key, op->key->len, entry);
    result->resolved_mutation = arena_alloc(result_arena, sizeof(SepticTankMutation));
    result->resolved_mutation->type = SEPTIC_TANK_SET;
    result->resolved_mutation->set = (SepticTankSetOperation) {
        .key = op->key,
        .value = op->value,
        .nx = op->nx,
        .xx = op->xx,
        .get = op->get,
    };

    if (expires_at.is_present) {
        result->resolved_mutation->set.expiration = (SepticTankExpiration) {
            .type = ST_EXPIRATION_UNIX_TS,
            .ts = expires_at.value,
        };
    } else {
        result->resolved_mutation->set.expiration = (SepticTankExpiration) {
            .type = ST_EXPIRATION_NO_EXPIRE,
            .ts = -1,
        };
    }

    return result;
}
