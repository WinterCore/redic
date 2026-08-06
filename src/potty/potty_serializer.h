#ifndef POTTY_SERIALIZER_H
#define POTTY_SERIALIZER_H

#include <stdio.h>

#include "../septic_tank/septic_tank_operation.h"

/*
 * On-disk AOF record format.
 *
 * Everything is written in the host's native byte order and width, so an AOF
 * is only portable between machines of the same endianness.
 *
 * Every record opens with a 1 byte tag holding the SepticTankOperationType
 * value, so reordering that enum silently invalidates every existing AOF.
 *
 * Strings are length prefixed and binary safe: uint64_t length followed by
 * exactly that many raw bytes, no NUL terminator.
 *
 *   SET    : [u8 tag][i64 expiration ts][str key][str value]
 *   DEL    : [u8 tag][str key]
 *   EXPIRE : [u8 tag][str key][i64 expires_at]
 *
 * A SET's expiry is an absolute unix ms timestamp, or ST_MUTATION_NO_EXPIRY
 * (-1) when the key has none. The tank resolves KEEP_OLD away long before a
 * mutation exists, so the record layout is exactly SepticTankSetMutation.
 */

/**
 * Appends the on-disk representation of `mutation` to `aof`.
 * Panics if any write comes up short.
 */
void potty_serialize_mutation(FILE *aof, SepticTankMutation *mutation);

#endif
