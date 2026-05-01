#include <pthread.h>
#include <stdlib.h>

#include "./septic_tank.h"


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

void *septic_tank_pump(void *input) {
    SepticTank *tank = input;
    
    // Load sewage indefinitely...

    pthread_mutex_lock(&tank->sewer->mutex);

    while (1) {
        SewerMessage *message = sewer_message_create(void *value, bool with_response);
        sewer_consume(tank->sewer, message);
    }
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
