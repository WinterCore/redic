#include <stdio.h>
#include <errno.h>

#include "potty_parser.h"
#include "potty_trainer.h"
#include "../aids.h"


PottyTrainer potty_trainer_create(SepticTank *septic_tank) {
    PottyTrainer trainer = {
        .septic_tank = septic_tank,
        .num_commands_processed = 0,
    };

    return trainer;
}

bool potty_trainer_train(PottyTrainer *potty_trainer) {
    FILE *aof = fopen("data.aof", "r");
    
    if (aof == NULL) {
        if (errno == ENOENT) {
            DEBUG_PRINTF("AOF file not found...");
            return true;
        }

        PANIC("Failed to open AOF!");
    }

    Arena *arena = arena_create();
    PottyParser parser = potty_parser_create(arena, aof);

    SepticTankMutation mutation = {0};

    while (1) {
        PottyParserReadResult result = potty_parser_read(&parser, &mutation);

        if (result == POTTY_PARSER_EOF) {
            return true;
        }

        if (result == POTTY_PARSER_READ_ERROR) {
            PANIC("Read error when replaying AOF");
        }

        if (result == POTTY_PARSER_UNEXPECTED_EOF) {
            PANIC("Unexpected AOF EOF");
        }

        potty_trainer->num_commands_processed += 1;
        septic_tank_apply_mutation(potty_trainer->septic_tank, &mutation);
        septic_tank_debug_mutation(&mutation);

        arena_reset(arena);
    }

    fclose(aof);

    return true;
}
