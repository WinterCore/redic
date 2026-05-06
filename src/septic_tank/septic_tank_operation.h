#ifndef SEPTIC_OPERATION_H
#define SEPTIC_OPERATION_H

#include <time.h>
#include <stdbool.h>

#include "septic_tank.h"
#include "../data.h"

typedef struct SepticTankExpiration {
    enum { ST_EXPIRATION_NO_EXPIRE, ST_EXPIRATION_UNIX_TS, ST_EXPIRATION_KEEP_OLD } type;
    time_t ts;
} SepticTankExpiration;

typedef struct SepticTankGetOperation {
    char *key;
} SepticTankGetOperation;

typedef struct SepticTankSetOperation {
    char *key;
    char *value;
    bool nx;
    bool xx;
    bool get;

    SepticTankExpiration expiration;
} SepticTankSetOperation;

typedef enum SepticTankOperationType {
    SEPTIC_TANK_SET,
    SEPTIC_TANK_GET,
} SepticTankOperationType;

typedef struct SepticTankOperation {
    // Used for allocating memory for values that will be passed
    // back
    Arena *arena;

    SepticTankOperationType type;
    union { 
        SepticTankSetOperation set;
        SepticTankGetOperation get;
    };
} SepticTankOperation;

typedef struct SepticTankGetResult {
    DataString *value;
} SepticTankGetResult;

typedef struct SepticTankSetResult {
    DataString *old_value;
} SepticTankSetResult;

typedef struct SepticTankResult {
    SepticTankOperationType type;
    union { 
        SepticTankSetResult set_result;
        SepticTankGetResult get_result;
    };

    bool success;
    char *error;
} SepticTankResult;

SepticTankResult *septic_tank_feed(
    Arena *arena,
    Sewer *septic_tank_sewer,
    SewerMessage *message
);

SepticTankResult *septic_tank_set(SepticTank *tank, Arena *result_arena, SepticTankSetOperation *op);
SepticTankResult *septic_tank_get(SepticTank *tank, Arena *result_arena, SepticTankGetOperation *op);

#endif
