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

typedef struct SepticTankExistsOperation {
    DataString *key;
} SepticTankExistsOperation;

typedef struct SepticTankTtlOperation {
    DataString *key;
} SepticTankTtlOperation;

typedef enum ExpireCondition {
    EXPIRE_COND_NONE,
    EXPIRE_COND_NX, // only set when the key has no existing expiry
    EXPIRE_COND_XX, // only set when the key already has an expiry
    EXPIRE_COND_GT, // only set when the new expiry is greater than the current one
    EXPIRE_COND_LT, // only set when the new expiry is less than the current one
} ExpireCondition;

typedef struct SepticTankExpireOperation {
    DataString *key;
    int64_t expires_at; // absolute unix ms; resolved by the command handler
    ExpireCondition condition; // resolved against the current expiry inside the tank
} SepticTankExpireOperation;

typedef struct SepticTankIncrByOperation {
    DataString *key;
    int64_t delta; // INCR=+1, DECR=-1, INCRBY=+n, DECRBY=-n
} SepticTankIncrByOperation;

typedef enum SepticTankOperationType {
    // Mutations
    SEPTIC_TANK_SET,
    SEPTIC_TANK_DEL,
    SEPTIC_TANK_EXPIRE,
    SEPTIC_TANK_INCRBY,

    SEPTIC_TANK_GET,
    SEPTIC_TANK_TTL,
    SEPTIC_TANK_EXISTS,
} SepticTankOperationType;

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
        SepticTankExistsOperation exists;
        SepticTankExpireOperation expire;
        SepticTankIncrByOperation incrby;
    };
} SepticTankOperation;

/*
 * Mutations are statements of fact about what the store should hold, where the
 * operations above are statements of intent. They carry none of the condition
 * flags (nx/xx/get/condition) because those are resolved away before a mutation
 * exists — that's what lets the AOF replay path apply one directly, and what
 * keeps them out of the file format.
 *
 * The field layout matches the AOF record layout one for one.
 */

// Stored in a mutation's expires_at when the key has no expiry
#define ST_MUTATION_NO_EXPIRY ((int64_t) -1)

typedef struct SepticTankSetMutation {
    DataString *key;
    DataString *value;
    int64_t expires_at; // absolute unix ms, or ST_MUTATION_NO_EXPIRY
} SepticTankSetMutation;

typedef struct SepticTankDelMutation {
    DataString *key;
} SepticTankDelMutation;

typedef struct SepticTankExpireMutation {
    DataString *key;
    int64_t expires_at; // absolute unix ms; an expiry already in the past resolves to a DEL
} SepticTankExpireMutation;

typedef struct SepticTankMutation {
    SepticTankOperationType type;

    union {
        SepticTankSetMutation set;
        SepticTankDelMutation del;
        SepticTankExpireMutation expire;
    };
} SepticTankMutation;


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

typedef struct SepticTankExistsResult {
    int64_t exists_count;
} SepticTankExistsResult;

typedef struct SepticTankExpireResult {
    int64_t is_set; // 1 if the timeout was applied, 0 if the key did not exist
} SepticTankExpireResult;

typedef struct SepticTankIncrByResult {
    int64_t value; // the value after applying the delta
} SepticTankIncrByResult;

typedef struct SepticTankResult {
    SepticTankOperationType type;
    union {
        SepticTankSetResult set_result;
        SepticTankGetResult get_result;
        SepticTankDelResult del_result;
        SepticTankTtlResult ttl_result;
        SepticTankExistsResult exists_result;
        SepticTankExpireResult expire_result;
        SepticTankIncrByResult incrby_result;
    };

    SepticTankMutation *resolved_mutation;

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

/*
 * The resolve_* functions read the tank and compute a result plus, when the
 * operation changes state, a `resolved_mutation` describing that change.
 * They never write it — septic_tank_apply_mutation does. They may still evict
 * expired entries, which is cache hygiene rather than a logged change.
 */

/**
 * Resolves SET semantics against the tank and returns a typed result.
 */
SepticTankResult *septic_tank_resolve_set(SepticTank *tank, Arena *result_arena, SepticTankSetOperation *op);

/**
 * Executes GET semantics against the tank and returns a typed result.
 */
SepticTankResult *septic_tank_get(SepticTank *tank, Arena *result_arena, SepticTankGetOperation *op);

/**
 * Resolves DEL semantics against the tank and returns a typed result.
 */
SepticTankResult *septic_tank_resolve_del(SepticTank *tank, Arena *result_arena, SepticTankDelOperation *op);

/**
 * Executes TTL semantics against the tank and returns a typed result.
 */
SepticTankResult *septic_tank_ttl(SepticTank *tank, Arena *result_arena, SepticTankTtlOperation *op);

/**
 * Executes EXISTS semantics against the tank and returns a typed result.
 */
SepticTankResult *septic_tank_exists(SepticTank *tank, Arena *result_arena, SepticTankExistsOperation *op);

/**
 * Resolves EXPIRE semantics against the tank and returns a typed result.
 * An expiry that already lies in the past resolves into a DEL mutation.
 */
SepticTankResult *septic_tank_resolve_expire(SepticTank *tank, Arena *result_arena, SepticTankExpireOperation *op);

/**
 * Resolves INCR/DECR/INCRBY/DECRBY semantics (signed delta) and returns a typed result.
 * Always resolves into a SET mutation — INCRBY is never persisted as itself.
 */
SepticTankResult *septic_tank_resolve_incrby(SepticTank *tank, Arena *result_arena, SepticTankIncrByOperation *op);

/*
 * The apply_* functions are the write half — the only code that touches the
 * tank's storage. Each lives beside the resolve_* it belongs to. They read only
 * mutation fields, never intent, which is what lets AOF replay call them.
 * INCRBY has none: it resolves into a SET.
 */

void septic_tank_apply_set(SepticTank *tank, SepticTankSetMutation *set);
void septic_tank_apply_del(SepticTank *tank, SepticTankDelMutation *del);
void septic_tank_apply_expire(SepticTank *tank, SepticTankExpireMutation *expire);

/**
 * Deep-clones a mutation payload into `arena` for async persistence/replay paths.
 */
SepticTankMutation *septic_tank_mutation_clone(Arena *arena, SepticTankMutation *mutation);

/**
 * Prints a mutation on one DEBUG_PRINTF line. Compiled out of release builds
 * along with the rest of DEBUG_PRINTF.
 */
void septic_tank_debug_mutation(SepticTankMutation *mutation);

#endif
