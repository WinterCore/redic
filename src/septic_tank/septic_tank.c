#include <pthread.h>
#include <stdlib.h>

#include "septic_tank.h"
#include "septic_tank_operation.h"
#include "../potty/potty.h"

SepticTank *septic_tank_create(Sewer *sewer, Potty *potty) {
    SepticTank *tank = malloc(sizeof(SepticTank));
    tank->data = hashmap_new_with_free(
        (hashmap_free_key_fn) free,
        (hashmap_free_value_fn) data_destroy_entry
    );
    tank->sewer = sewer;
    tank->potty = potty;

    return tank;
}

void septic_tank_destroy(SepticTank *tank) {
    hashmap_free(tank->data);
    free(tank);
}

void septic_tank_digest(SepticTank *tank, SewerMessage *message) {
    SepticTankOperation *operation = message->value;
    
    SepticTankResult *result;
    switch (operation->type) {
        case SEPTIC_TANK_GET: {
            result = septic_tank_get(tank, operation->response_arena, &operation->get);
            break;
        }
        case SEPTIC_TANK_SET: {
            result = septic_tank_set(tank, operation->response_arena, &operation->set);
            break;
        }
        case SEPTIC_TANK_DEL: {
            result = septic_tank_del(tank, operation->response_arena, &operation->del);
            break;
        }
        case SEPTIC_TANK_TTL: {
            result = septic_tank_ttl(tank, operation->response_arena, &operation->ttl);
            break;
        }
        default:
            UNIMPLEMENTED("Unknown digest operation %d", operation->type);
    }

    bool succeeded = result->success;

    if (message->clogged_sewer) {
        SewerMessage *response_message = sewer_message_create(operation->response_arena, result, false);

        // Ideally sewer_send never blocks, it would be a big no no if it did.
        // We'd end up with lots of poo poo
        sewer_send(message->clogged_sewer, response_message);
        response_message->is_consumed = true;
    }

    if (succeeded && septic_tank_operation_is_mutation(operation->type)) {
        SepticTankMutation mutation = septic_tank_mutation_from_operation(operation);

        potty_feed(tank->potty, &mutation);
    }
}

void *septic_tank_pump(void *input) {
    SepticTank *tank = input;
    
    // Load sewage indefinitely...
    while (1) {
        SewerMessage *message = sewer_consume(tank->sewer);
        septic_tank_digest(tank, message);

        sewer_message_destroy(message, false);
    }
}

pthread_t septic_tank_launch(SepticTank *tank) {
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&tid, &attr, septic_tank_pump, tank);
    pthread_attr_destroy(&attr);

    return tid;
}
