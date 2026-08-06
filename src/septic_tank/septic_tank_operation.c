#include <stdint.h>
#include <string.h>
#include "septic_tank_operation.h"

static SepticTankOperation *clone_operation(Arena *arena, SepticTankOperation *operation) {
    SepticTankOperation *cloned_operation = arena_alloc(arena, sizeof(SepticTankOperation));

    memcpy(cloned_operation, operation, sizeof(SepticTankOperation));

    switch (operation->type) {
        case SEPTIC_TANK_DEL:
            cloned_operation->del.key = data_copy_string_arena(arena, operation->del.key);
            break;
        case SEPTIC_TANK_SET:
            cloned_operation->set.key = data_copy_string_arena(arena, operation->set.key);
            cloned_operation->set.value = data_copy_string_arena(arena, operation->set.value);
            break;
        case SEPTIC_TANK_TTL:
            cloned_operation->ttl.key = data_copy_string_arena(arena, operation->ttl.key);
            break;
        case SEPTIC_TANK_GET:
            cloned_operation->get.key = data_copy_string_arena(arena, operation->get.key);
            break;
        case SEPTIC_TANK_EXISTS:
            cloned_operation->exists.key = data_copy_string_arena(arena, operation->exists.key);
            break;
        case SEPTIC_TANK_EXPIRE:
            cloned_operation->expire.key = data_copy_string_arena(arena, operation->expire.key);
            break;
        case SEPTIC_TANK_INCRBY:
            cloned_operation->incrby.key = data_copy_string_arena(arena, operation->incrby.key);
            break;
    }

    return cloned_operation;
}

SepticTankResult *septic_tank_feed(
    Sewer *septic_tank_sewer,
    SepticTankOperation *operation
) {
    Arena *message_arena = arena_create();
    SepticTankOperation *cloned_operation = clone_operation(message_arena, operation);
    SewerMessage *message = sewer_message_create(message_arena, cloned_operation, true);

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
        case SEPTIC_TANK_GET:
        case SEPTIC_TANK_TTL:
        case SEPTIC_TANK_EXISTS:
        case SEPTIC_TANK_INCRBY:
            // Not a persisted mutation type
            UNREACHABLE();
            break;
        case SEPTIC_TANK_SET:
            clone->set.key = data_copy_string_arena(arena, mutation->set.key);
            clone->set.value = data_copy_string_arena(arena, mutation->set.value);
            break;
        case SEPTIC_TANK_DEL:
            clone->del.key = data_copy_string_arena(arena, mutation->del.key);
            break;
        case SEPTIC_TANK_EXPIRE:
            clone->expire.key = data_copy_string_arena(arena, mutation->expire.key);
            break;
    }

    return clone;
}
