#ifndef POTTY_TRAINER_H
#define POTTY_TRAINER_H

#include "../septic_tank/septic_tank.h"

/*
 * Potty trainer is the AOF replay implementation
 */

typedef struct PottyTrainer {
    SepticTank *septic_tank;
} PottyTrainer;

PottyTrainer potty_trainer_create(SepticTank *septic_tank);
bool potty_trainer_train(PottyTrainer *potty_trainer);

#endif
