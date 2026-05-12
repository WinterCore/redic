#ifndef SEPTIC_TANK_H
#define SEPTIC_TANK_H

#include <pthread.h>
#include "../hashmap.h"
#include "../sewer.h"

typedef struct Potty Potty;

typedef struct SepticTank {
    map_t data;

    Sewer *sewer;
    Potty *potty;
} SepticTank;

/**
 * Allocates a septic tank instance bound to `sewer`.
 * The tank owns its internal hashmap; the caller owns `sewer`.
 */
SepticTank *septic_tank_create(Sewer *sewer, Potty *potty);

/**
 * Frees tank storage and all in-memory key/value entries.
 */
void septic_tank_destroy(SepticTank *tank);

/**
 * Starts the detached septic tank worker thread that consumes from `sewer`.
 * Returns the created thread id.
 */
pthread_t septic_tank_launch(SepticTank *tank);

#endif
