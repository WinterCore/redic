#include "septic_tank_operation.h"


SepticTankResult *septic_tank_feed(
    Arena *arena,
    Sewer *septic_tank_sewer,
    SewerMessage *message
) {
    // Send
    sewer_send(septic_tank_sewer, message);

    // Wait for response
    SewerMessage *response_message = arena_alloc(arena, sizeof(SewerMessage));
    sewer_consume(message->clogged_sewer, response_message);

    SepticTankResult *result = response_message->value;

    return result;
}
