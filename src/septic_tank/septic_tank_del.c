#include "septic_tank_operation.h"
#include "septic_tank.h"

SepticTankResult *septic_tank_del(SepticTank *tank, Arena *result_arena, SepticTankDelOperation *op) {
    SepticTankResult *result = arena_alloc(result_arena, sizeof(SepticTankResult));
    result->type = SEPTIC_TANK_DEL;
    result->success = false;
    result->del_result = (SepticTankDelResult) { .num_deleted = 0 };

    DataEntry *entry = NULL;
    int get_result = hashmap_get(tank->data, op->key->str, op->key->len, (void **) &entry);

    if (get_result == MAP_OK && !data_is_expired(entry)) {
        hashmap_remove(tank->data, op->key->str, op->key->len);
        result->del_result.num_deleted = 1;
        result->success = true;
    }

    return result;
}
