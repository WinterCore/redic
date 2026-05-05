#ifndef SEPTIC_TANK_H
#define SEPTIC_TANK_H

#include <pthread.h>
#include "../hashmap.h"
#include "../sewer.h"

typedef struct SepticTank {
    map_t data;

    Sewer *sewer;
} SepticTank;

SepticTank *septic_tank_create(Sewer *sewer);
void septic_tank_destroy(SepticTank *tank);

pthread_t septic_tank_launch(Sewer *sewer);

#endif
