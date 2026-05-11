#include "./septic_tank_operation.h"

SepticTankResult *septic_tank_get(SepticTank *tank, Arena *result_arena, SepticTankGetOperation *op) {
    SepticTankResult *result = arena_alloc(result_arena, sizeof(SepticTankResult));
    result->type = SEPTIC_TANK_GET;

    DataEntry *entry = NULL;
    int get_result = hashmap_get(tank->data, op->key, (void **) &entry);

    if (get_result == MAP_MISSING) {
        result->success = true;
        result->get_result.value = NULL;
        return result;
    }

    if (data_is_expired(entry)) {
        result->success = true;
        result->get_result.value = NULL;
        hashmap_remove(tank->data, op->key);

        return result;
    }

    if (entry->type != DATA_STRING) {
        result->success = false;
        result->error = "WRONGTYPE Operation against a key holding the wrong kind of value";
        return result;
    }

    DataString *value = data_unwrap_string(entry);
    result->success = true;
    result->get_result.value = data_copy_string_arena(result_arena, value);

    return result;
}
