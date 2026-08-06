#include "septic_tank.h"
#include "septic_tank_operation.h"

/*
 * Dispatch for the write half of the tank. The apply_* functions live beside the
 * resolve_* half they belong to, in each operation's own file; this is only the
 * switch that picks one, plus the entry point AOF replay comes in through.
 */
void septic_tank_apply_mutation(SepticTank *tank, SepticTankMutation *mutation) {
    switch (mutation->type) {
        case SEPTIC_TANK_SET:
            septic_tank_apply_set(tank, &mutation->set);
            break;

        case SEPTIC_TANK_DEL:
            septic_tank_apply_del(tank, &mutation->del);
            break;

        case SEPTIC_TANK_EXPIRE:
            septic_tank_apply_expire(tank, &mutation->expire);
            break;

        case SEPTIC_TANK_INCRBY:
            // Resolved into SET
            UNREACHABLE();

        case SEPTIC_TANK_GET:
        case SEPTIC_TANK_TTL:
        case SEPTIC_TANK_EXISTS:
            // Not persisted mutation types
            UNREACHABLE();
    }
}

void septic_tank_replay_mutation(SepticTank *tank, SepticTankMutation *mutation) {
    septic_tank_apply_mutation(tank, mutation);
}
