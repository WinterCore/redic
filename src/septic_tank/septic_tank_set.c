#include <_string.h>
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

    if (op->nx || op->xx || op->get || op->expiration.type == ST_EXPIRATION_KEEP_OLD) {
        int get_result = hashmap_get(tank->data, op->key, (void **) &old_entry);

        // Set only works with strings
        if (get_result == MAP_OK && old_entry->type != DATA_STRING) {
            result->error = "WRONGTYPE Operation against a key holding the wrong kind of value";
            return result;
        }

        if (get_result == MAP_OK && data_is_expired(old_entry)) {
            hashmap_remove(tank->data, op->key);
            free(old_entry);
            old_entry = NULL;
            return result;
        }

        DEBUG_PRINTF("nx %d, xx %d, get_result %d", op->nx, op->xx, get_result);

        if (
            (op->nx && get_result == MAP_OK) ||
            (op->xx && get_result == MAP_MISSING)
        ) {
            if (op->get && old_entry != NULL) {
                result->success = true;
                DataString *old_value = data_unwrap_string(old_entry);
                result->set_result.old_value = data_copy_string_arena(result_arena, old_value);
            }

            result->success = false;
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

    // Insert new entry
    DataEntry *entry = data_create_string_entry(expires_at, strlen(op->value), op->value);
    char *key = strdup(op->key);
    hashmap_put(tank->data, key, entry);
    result->success = true;

    if (op->get && old_entry != NULL) {
        DataString *old_value = data_unwrap_string(old_entry);
        result->set_result.old_value = data_copy_string_arena(result_arena, old_value);
    }
    
    if (old_entry != NULL) {
        free(old_entry);
    }

    return result;
}
