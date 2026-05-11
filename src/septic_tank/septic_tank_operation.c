#include <string.h>
#include "septic_tank_operation.h"


SepticTankResult *septic_tank_feed(
    Sewer *septic_tank_sewer,
    SewerMessage *message
) {
    Sewer *response_sewer = message->clogged_sewer;

    // Send
    sewer_send(septic_tank_sewer, message);

    // Wait for response
    SewerMessage *response_message = sewer_consume(response_sewer);
    sewer_destroy(response_sewer);

    SepticTankResult *result = response_message->value;

    return result;
}

SepticTankMutation *septic_tank_mutation_clone(Arena *arena, SepticTankMutation *mutation) {
    SepticTankMutation *clone = arena_alloc(arena, sizeof(SepticTankMutation));
    memcpy(clone, mutation, sizeof(SepticTankMutation));

    switch (mutation->type) {
        case SEPTIC_TANK_SET:
            clone->set.key = clone_cstr(arena, mutation->set.key);
            clone->set.value = clone_cstr(arena, mutation->set.value);
            break;
        case SEPTIC_TANK_DEL:
            clone->del.key = clone_cstr(arena, mutation->del.key);
            break;
        default:
            UNREACHABLE("Not a mutation");
    }

    return clone;
}
