#include "septic_tank_operation.h"

SepticTankResult *septic_tank_ttl(SepticTank *tank, Arena *result_arena, SepticTankTtlOperation *op) {
    SepticTankResult *result = arena_alloc(result_arena, sizeof(SepticTankResult));
    result->type = SEPTIC_TANK_GET;
    result->success = true;

    DataEntry *entry = NULL;
    int get_result = hashmap_get(tank->data, op->key, (void **) &entry);
    
    if (get_result == MAP_MISSING) {
        result->ttl_result.ttl_s = -2;
        return result;
    }

    if (!entry->expires_at.is_present) {
        result->ttl_result.ttl_s = -1;
        return result;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    result->ttl_result.ttl_s = (entry->expires_at.value / 1000) - (int64_t) ts.tv_sec;

    return result;
}
