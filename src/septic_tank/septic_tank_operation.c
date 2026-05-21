#include <string.h>
#include "septic_tank_operation.h"

bool septic_tank_operation_is_mutation(SepticTankOperationType type) {
    return type == SEPTIC_TANK_SET || type == SEPTIC_TANK_DEL;
}

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
        default:
            UNREACHABLE();
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
        case SEPTIC_TANK_SET:
            clone->set.key = data_copy_string_arena(arena, mutation->set.key);
            clone->set.value = data_copy_string_arena(arena, mutation->set.value);
            break;
        case SEPTIC_TANK_DEL:
            clone->del.key = data_copy_string_arena(arena, mutation->del.key);
            break;
        default:
            UNREACHABLE();
    }

    return clone;
}

SepticTankMutation septic_tank_mutation_from_operation(SepticTankOperation *operation) {
    SepticTankMutation mutation = {};
    mutation.type = operation->type;

    switch (operation->type) {
        case SEPTIC_TANK_SET:
            mutation.set = operation->set;
            break;
        case SEPTIC_TANK_DEL:
            mutation.del = operation->del;
            break;
        default:
            UNREACHABLE();
    }

    return mutation;
}
