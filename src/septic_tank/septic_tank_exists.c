#include "septic_tank_operation.h"
#include "septic_tank.h"

SepticTankResult *septic_tank_exists(SepticTank *tank, Arena *result_arena, SepticTankExistsOperation *op) {
    SepticTankResult *result = arena_alloc(result_arena, sizeof(SepticTankResult));
    result->error = NULL;
    result->type = SEPTIC_TANK_EXISTS;
    result->success = true;
    result->resolved_mutation = NULL;
    result->exists_result = (SepticTankExistsResult) { .exists_count = 0 };

    DataEntry *entry = NULL;
    int get_result = hashmap_get(tank->data, op->key->str, op->key->len, (void **) &entry);

    if (get_result == MAP_OK) {
        bool is_expired = data_is_expired(entry);

        if (!is_expired) {
            result->exists_result.exists_count = 1;
        }

        if (is_expired) {
            hashmap_remove(tank->data, op->key->str, op->key->len);
        }
    }

    return result;
}
