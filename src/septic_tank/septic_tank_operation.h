#ifndef SEPTIC_OPERATION_H
#define SEPTIC_OPERATION_H

#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>

#include "../data.h"
#include "../sewer.h"

typedef struct SepticTank SepticTank;

typedef struct SepticTankExpiration {
    enum { ST_EXPIRATION_NO_EXPIRE, ST_EXPIRATION_UNIX_TS, ST_EXPIRATION_KEEP_OLD } type;
    int64_t ts;
} SepticTankExpiration;

typedef struct SepticTankGetOperation {
    DataString *key;
} SepticTankGetOperation;

typedef struct SepticTankSetOperation {
    DataString *key;
    DataString *value;
    bool nx;
    bool xx;
    bool get;

    SepticTankExpiration expiration;
} SepticTankSetOperation;

typedef struct SepticTankDelOperation {
    DataString *key;
} SepticTankDelOperation;

typedef struct SepticTankTtlOperation {
    DataString *key;
} SepticTankTtlOperation;

typedef enum SepticTankOperationType {
    // Mutations
    SEPTIC_TANK_SET,
    SEPTIC_TANK_DEL,

    SEPTIC_TANK_GET,
    SEPTIC_TANK_TTL,
} SepticTankOperationType;

/**
 * Returns true when operation type represents a persisted mutation (SET/DEL).
 */
bool septic_tank_operation_is_mutation(SepticTankOperationType type);

typedef struct SepticTankOperation {
    // Used for allocating memory for values that will be passed
    // back
    Arena *response_arena;

    SepticTankOperationType type;
    union { 
        SepticTankSetOperation set;
        SepticTankGetOperation get;
        SepticTankDelOperation del;
        SepticTankTtlOperation ttl;
    };
} SepticTankOperation;

typedef struct SepticTankGetResult {
    DataString *value;
} SepticTankGetResult;

typedef struct SepticTankSetResult {
    DataString *old_value;
} SepticTankSetResult;

typedef struct SepticTankDelResult {
    uint32_t num_deleted;
} SepticTankDelResult;

typedef struct SepticTankTtlResult {
    int64_t ttl_s;  // -1 = key exists but has no expiry, -2 = key does not exist
} SepticTankTtlResult;

typedef struct SepticTankResult {
    SepticTankOperationType type;
    union { 
        SepticTankSetResult set_result;
        SepticTankGetResult get_result;
        SepticTankDelResult del_result;
        SepticTankTtlResult ttl_result;
    };

    bool success;
    char *error;
} SepticTankResult;

/**
 * Sends an operation to the septic tank actor and blocks for a response.
 * The operation is cloned into a message-owned arena before enqueueing.
 * Returned result memory is allocated from `operation->response_arena`.
 */
SepticTankResult *septic_tank_feed(
    Sewer *septic_tank_sewer,
    SepticTankOperation *operation
);

/**
 * Executes SET semantics against the tank and returns a typed result.
 */
SepticTankResult *septic_tank_set(SepticTank *tank, Arena *result_arena, SepticTankSetOperation *op);

/**
 * Executes GET semantics against the tank and returns a typed result.
 */
SepticTankResult *septic_tank_get(SepticTank *tank, Arena *result_arena, SepticTankGetOperation *op);

/**
 * Executes DEL semantics against the tank and returns a typed result.
 */
SepticTankResult *septic_tank_del(SepticTank *tank, Arena *result_arena, SepticTankDelOperation *op);

/**
 * Executes TTL semantics against the tank and returns a typed result.
 */
SepticTankResult *septic_tank_ttl(SepticTank *tank, Arena *result_arena, SepticTankTtlOperation *op);


typedef struct SepticTankMutation {
    SepticTankOperationType type;

    union { 
        SepticTankSetOperation set;
        SepticTankDelOperation del;
    };
} SepticTankMutation;

/**
 * Deep-clones a mutation payload into `arena` for async persistence/replay paths.
 */
SepticTankMutation *septic_tank_mutation_clone(Arena *arena, SepticTankMutation *mutation);

/**
 * Derives a mutation payload from a write operation (SET/DEL) using shallow
 * field copies into a new `SepticTankMutation` allocated in `arena`.
 */
SepticTankMutation septic_tank_mutation_from_operation(SepticTankOperation *operation);

#endif
