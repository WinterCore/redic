#include "septic_tank_operation.h"
#include "septic_tank.h"

SepticTankResult *septic_tank_del(SepticTank *tank, Arena *result_arena, SepticTankDelOperation *op) {
    SepticTankResult *result = arena_alloc(result_arena, sizeof(SepticTankResult));
    result->error = NULL;
    result->type = SEPTIC_TANK_DEL;
    result->success = true;
    result->resolved_mutation = NULL;
    result->del_result = (SepticTankDelResult) { .num_deleted = 0 };

    DataEntry *entry = NULL;
    int get_result = hashmap_get(tank->data, op->key->str, op->key->len, (void **) &entry);

    if (get_result == MAP_OK) {
        bool is_expired = data_is_expired(entry);
        hashmap_remove(tank->data, op->key->str, op->key->len);

        if (!is_expired) {
            result->del_result.num_deleted = 1;
            result->resolved_mutation = arena_alloc(result_arena, sizeof(SepticTankMutation));
            result->resolved_mutation->type = SEPTIC_TANK_DEL;
            result->resolved_mutation->del = (SepticTankDelOperation) { .key = op->key };
        }
    }

    return result;
}
