#ifndef SEPTIC_TANK_H
#define SEPTIC_TANK_H

#include <pthread.h>
#include "../hashmap.h"
#include "../sewer.h"
#include "septic_tank_operation.h"

typedef struct Potty Potty;

typedef struct SepticTank {
    map_t data;

    Sewer *sewer;
    Potty *potty;

    bool aof_enabled;
} SepticTank;

/**
 * Allocates a septic tank instance bound to `sewer`.
 * The tank owns its internal hashmap; the caller owns `sewer`.
 */
SepticTank *septic_tank_create(Sewer *sewer, Potty *potty);

void septic_tank_disable_aof(SepticTank *st);
void septic_tank_enable_aof(SepticTank *st);

/**
 * Frees tank storage and all in-memory key/value entries.
 */
void septic_tank_destroy(SepticTank *tank);

/**
 * Starts the detached septic tank worker thread that consumes from `sewer`.
 * Returns the created thread id.
 */
pthread_t septic_tank_launch(SepticTank *tank);

/**
 * Applies a resolved mutation to the tank's storage. The only writer.
 * Reads none of the intent fields (nx/xx/get/condition), which is what lets the
 * AOF replay path reuse it without persisting them.
 */
void septic_tank_apply_mutation(SepticTank *tank, SepticTankMutation *mutation);

/**
 * Used for replaying AOF mutations
 * WARN: Not thread safe, only call upon initiatlization, for multithreading use
 * the septic tank sewer instead
 */
void septic_tank_replay_mutation(SepticTank *tank, SepticTankMutation *mutation);

#endif
