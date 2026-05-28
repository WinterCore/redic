#include "septic_tank_operation.h"
#include "septic_tank.h"

SepticTankResult *septic_tank_expire(SepticTank *tank, Arena *result_arena, SepticTankExpireOperation *op) {
    SepticTankResult *result = arena_alloc(result_arena, sizeof(SepticTankResult));
    result->type = SEPTIC_TANK_EXPIRE;
    result->resolved_mutation = NULL;
    result->error = NULL;
    result->success = true;
    result->expire_result = (SepticTankExpireResult) { .is_set = 0 };
    
    DataEntry *entry = NULL;

    hashmap_get(tank->data, op->key->str, op->key->len, (void **) &entry);

    if (entry == NULL) {
        return result;
    }

    if (data_is_expired(entry)) {
        hashmap_remove(tank->data, op->key->str, op->key->len);
        return result;
    }

    bool has_expiry = entry->expires_at.is_present;
    
    // Noops
    if (
        (op->condition == EXPIRE_COND_NX && has_expiry) ||
        (op->condition == EXPIRE_COND_XX && !has_expiry) ||
        (op->condition == EXPIRE_COND_GT && !has_expiry) ||
        (op->condition == EXPIRE_COND_GT && has_expiry && op->expires_at <= entry->expires_at.value) ||
        (op->condition == EXPIRE_COND_LT && has_expiry && op->expires_at >= entry->expires_at.value)
    ) {
        return result;
    }

    entry->expires_at.is_present = true;
    entry->expires_at.value = op->expires_at;
    result->expire_result.is_set = true;

    result->resolved_mutation = arena_alloc(result_arena, sizeof(SepticTankMutation));
    result->resolved_mutation->type = SEPTIC_TANK_EXPIRE;
    result->resolved_mutation->expire = (SepticTankExpireOperation) {
        .key = op->key,
        .expires_at = op->expires_at,
        .condition = EXPIRE_COND_NONE,
    };

    if (data_is_expired(entry)) {
        hashmap_remove(tank->data, op->key->str, op->key->len);
        result->resolved_mutation->type = SEPTIC_TANK_DEL;
        result->resolved_mutation->del = (SepticTankDelOperation) { .key = op->key };
    }
    

    return result;
}
