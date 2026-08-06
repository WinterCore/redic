#include "septic_tank_operation.h"
#include "septic_tank.h"
#include "../data.h"

SepticTankResult *septic_tank_resolve_expire(SepticTank *tank, Arena *result_arena, SepticTankExpireOperation *op) {
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

    result->expire_result.is_set = true;
    result->resolved_mutation = arena_alloc(result_arena, sizeof(SepticTankMutation));

    // An expiry that's already in the past is a delete, not an expiry. The old
    // code discovered this by writing the value and re-inspecting the entry;
    // resolve has to predict it instead, since it doesn't write.
    if (op->expires_at <= data_now_ms()) {
        result->resolved_mutation->type = SEPTIC_TANK_DEL;
        result->resolved_mutation->del = (SepticTankDelMutation) { .key = op->key };

        return result;
    }

    result->resolved_mutation->type = SEPTIC_TANK_EXPIRE;
    result->resolved_mutation->expire = (SepticTankExpireMutation) {
        .key = op->key,
        .expires_at = op->expires_at,
    };

    return result;
}

void septic_tank_apply_expire(SepticTank *tank, SepticTankExpireMutation *expire) {
    DataEntry *entry = NULL;

    int get_result = hashmap_get(
        tank->data,
        expire->key->str,
        expire->key->len,
        (void **) &entry
    );

    // Key went away between resolve and apply, or the log outlived it
    if (get_result != MAP_OK) {
        return;
    }

    entry->expires_at.is_present = true;
    entry->expires_at.value = expire->expires_at;
}
