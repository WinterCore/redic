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
    result->error = NULL;
    result->success = false;

    int get_result = hashmap_get(tank->data, op->key->str, op->key->len, (void **) &old_entry);

    if (op->nx || op->xx || op->get || op->expiration.type == ST_EXPIRATION_KEEP_OLD) {
        // Set only works with strings
        if (get_result == MAP_OK && old_entry->type != DATA_STRING) {
            result->error = "WRONGTYPE Operation against a key holding the wrong kind of value";
            return result;
        }

        if (get_result == MAP_OK && data_is_expired(old_entry)) {
            hashmap_remove(tank->data, op->key->str, op->key->len);
            old_entry = NULL;
            return result;
        }

        if (
            (op->nx && get_result == MAP_OK) ||
            (op->xx && get_result == MAP_MISSING)
        ) {
            if (op->get && old_entry != NULL) {
                DataString *old_value = data_unwrap_string(old_entry);
                result->set_result.old_value = data_copy_string_arena(result_arena, old_value);
            }

            return result;
        }
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
        default:
            UNREACHABLE();
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

    result->success = true;

    return result;
}
