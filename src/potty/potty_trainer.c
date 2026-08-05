#include <stdio.h>

#include "./potty_trainer.h"
#include "../aids.h"


PottyTrainer potty_trainer_create(SepticTank *septic_tank) {
    PottyTrainer trainer = {
        .septic_tank = septic_tank
    };
}

bool potty_trainer_train(PottyTrainer *potty_trainer) {
    FILE *aof = fopen("data.aof", "a");
    
    if (aof == NULL) {
        PANIC("Failed to open AOF!");
    }

    // fread(void *, unsigned long, unsigned long, FILE *);
    UNIMPLEMENTED("");

    fclose(aof);

    return false;
}
