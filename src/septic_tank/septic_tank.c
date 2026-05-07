#include <pthread.h>
#include <stdlib.h>

#include "./septic_tank.h"
#include "septic_tank_operation.h"

SepticTank *septic_tank_create(Sewer *sewer) {
    SepticTank *tank = malloc(sizeof(SepticTank));
    tank->data = hashmap_new();
    tank->sewer = sewer;

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
            result = septic_tank_get(tank, operation->arena, &operation->get);
            break;
        }
        case SEPTIC_TANK_SET: {
            result = septic_tank_set(tank, operation->arena, &operation->set);
            break;
        }
        case SEPTIC_TANK_DEL: {
            result = septic_tank_del(tank, operation->arena, &operation->del);
            break;
        }
        case SEPTIC_TANK_TTL: {
            result = septic_tank_ttl(tank, operation->arena, &operation->ttl);
            break;
        }
        default:
            UNIMPLEMENTED("Unknown digest operation %d", operation->type);
    }

    if (message->clogged_sewer) {
        SewerMessage *response_message = sewer_message_create(operation->arena, result, false);

        // Ideally sewer_send never blocks, it would be a big no no if it did.
        // We'd end up with lots of poo poo
        sewer_send(message->clogged_sewer, response_message);
        response_message->is_consumed = true;
    }
}

void *septic_tank_pump(void *input) {
    SepticTank *tank = input;
    SewerMessage *message = malloc(sizeof(SewerMessage));
    
    // Load sewage indefinitely...
    while (1) {
        sewer_consume(tank->sewer, message);
        septic_tank_digest(tank, message);
    }

    free(message);
}

pthread_t septic_tank_launch(Sewer *sewer) {
    SepticTank *tank = septic_tank_create(sewer);

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&tid, &attr, septic_tank_pump, tank);
    pthread_attr_destroy(&attr);

    return tid;
}
