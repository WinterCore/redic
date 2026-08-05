// test.ts
import { connect, ErrorReplyError } from "@db/redis";
import { assertEquals, assertExists, assertRejects } from "@std/assert";

async function makeRedis() {
  return await connect({ hostname: "127.0.0.1", port: 6969 });
}

function rkey(prefix: string) {
  return `${prefix}_${crypto.randomUUID().replace(/-/g, "").slice(0, 8)}`;
}

Deno.test("set and get", async () => {
  const redis = await makeRedis();
  const key = rkey("set_get");

  await redis.set(key, "bar");
  const value = await redis.get(key);
  assertEquals(value, "bar", "expected value to be 'bar'");

  redis.close();
});

Deno.test("get missing key returns null", async () => {
  const redis = await makeRedis();

  const value = await redis.get(rkey("missing"));
  assertEquals(value, null, "expected missing key to return null");

  redis.close();
});

Deno.test("set NX - only sets if key does not exist", async () => {
  const redis = await makeRedis();
  const key = rkey("nx");

  const first = await redis.set(key, "first", { nx: true });
  assertEquals(first, "OK", "NX on new key should return OK");

  const second = await redis.set(key, "second", { nx: true });
  assertEquals(second, null, "NX on existing key should return null");

  const value = await redis.get(key);
  assertEquals(value, "first", "value should not have been overwritten by NX");

  redis.close();
});

Deno.test("set XX - only sets if key exists", async () => {
  const redis = await makeRedis();
  const missingKey = rkey("xx_missing");
  const existingKey = rkey("xx_existing");

  const first = await redis.set(missingKey, "value", { xx: true });
  assertEquals(first, null, "XX on missing key should return null");

  await redis.set(existingKey, "initial");
  const second = await redis.set(existingKey, "updated", { xx: true });
  assertEquals(second, "OK", "XX on existing key should return OK");

  const value = await redis.get(existingKey);
  assertEquals(value, "updated", "value should have been updated by XX");

  redis.close();
});

Deno.test("set GET - returns old value", async () => {
  const redis = await makeRedis();
  const key = rkey("get_opt");

  await redis.set(key, "old");
  const old = await redis.set(key, "new", { get: true });
  assertEquals(old, "old", "GET option should return old value");

  const current = await redis.get(key);
  assertEquals(current, "new", "key should have new value after set");

  redis.close();
});

Deno.test("set GET on missing key returns null", async () => {
  const redis = await makeRedis();

  const old = await redis.set(rkey("get_missing"), "value", { get: true });
  assertEquals(old, null, "GET option on missing key should return null");

  redis.close();
});

Deno.test("set EX - key expires after seconds", async () => {
  const redis = await makeRedis();
  const key = rkey("ex");

  await redis.set(key, "value", { ex: 1 });

  const before = await redis.get(key);
  assertEquals(before, "value", "key should exist before expiry");

  await new Promise((r) => setTimeout(r, 1100));

  const after = await redis.get(key);
  assertEquals(after, null, "key should be gone after expiry");

  redis.close();
});

Deno.test("set PX - key expires after milliseconds", async () => {
  const redis = await makeRedis();
  const key = rkey("px");

  await redis.set(key, "value", { px: 500 });

  const before = await redis.get(key);
  assertEquals(before, "value", "key should exist before expiry");

  await new Promise((r) => setTimeout(r, 600));

  const after = await redis.get(key);
  assertEquals(after, null, "key should be gone after expiry");

  redis.close();
});

Deno.test("set KEEPTTL - preserves existing TTL", async () => {
  const redis = await makeRedis();
  const key = rkey("keepttl");

  await redis.set(key, "original", { ex: 10 });
  await redis.set(key, "updated", { keepttl: true });

  const ttl = await redis.ttl(key);
  assertExists(ttl, "TTL should exist after KEEPTTL");
  assertEquals(ttl > 0, true, "TTL should still be positive after KEEPTTL");

  const value = await redis.get(key);
  assertEquals(value, "updated", "value should be updated after KEEPTTL set");

  redis.close();
});

Deno.test("set EXAT - key expires at an absolute time", async () => {
  const redis = await makeRedis();
  const key = rkey("exat");

  const exat = Math.floor(Date.now() / 1000) + 100;
  await redis.set(key, "value", { exat });

  const ttl = await redis.ttl(key);
  assertExists(ttl, "TTL should exist after EXAT");
  assertEquals(ttl > 0 && ttl <= 100, true, "EXAT should yield a positive TTL within range");

  redis.close();
});

Deno.test("incr - initializes a missing key to 1", async () => {
  const redis = await makeRedis();
  const key = rkey("incr_new");

  assertEquals(await redis.incr(key), 1, "incr on a missing key should return 1");

  redis.close();
});

Deno.test("incr - increments an existing counter", async () => {
  const redis = await makeRedis();
  const key = rkey("incr");

  await redis.set(key, "10");
  assertEquals(await redis.incr(key), 11);
  assertEquals(await redis.get(key), "11", "stored value should round-trip as a string");

  redis.close();
});

Deno.test("decr - initializes a missing key to -1", async () => {
  const redis = await makeRedis();
  const key = rkey("decr_new");

  assertEquals(await redis.decr(key), -1, "decr on a missing key should return -1");

  redis.close();
});

Deno.test("decr - decrements an existing counter", async () => {
  const redis = await makeRedis();
  const key = rkey("decr");

  await redis.set(key, "10");
  assertEquals(await redis.decr(key), 9);

  redis.close();
});

Deno.test("incrby - adds the given amount", async () => {
  const redis = await makeRedis();
  const key = rkey("incrby");

  await redis.set(key, "5");
  assertEquals(await redis.incrby(key, 20), 25);

  redis.close();
});

Deno.test("incrby - missing key starts from 0", async () => {
  const redis = await makeRedis();
  const key = rkey("incrby_new");

  assertEquals(await redis.incrby(key, 7), 7);

  redis.close();
});

Deno.test("decrby - subtracts the given amount", async () => {
  const redis = await makeRedis();
  const key = rkey("decrby");

  await redis.set(key, "20");
  assertEquals(await redis.decrby(key, 5), 15);

  redis.close();
});

Deno.test("decrby - a negative decrement increments", async () => {
  const redis = await makeRedis();
  const key = rkey("decrby_neg");

  await redis.set(key, "10");
  assertEquals(await redis.decrby(key, -5), 15, "DECRBY by a negative amount should increment");

  redis.close();
});

Deno.test("incr - preserves an existing TTL", async () => {
  const redis = await makeRedis();
  const key = rkey("incr_ttl");

  await redis.set(key, "1", { ex: 100 });
  await redis.incr(key);

  const ttl = await redis.ttl(key);
  assertExists(ttl, "TTL should exist after incr");
  assertEquals(ttl > 0, true, "TTL should survive an increment");

  redis.close();
});

Deno.test("incr - errors on a non-integer value", async () => {
  const redis = await makeRedis();
  const key = rkey("incr_nan");

  await redis.set(key, "not a number");
  await assertRejects(() => redis.incr(key), ErrorReplyError);

  redis.close();
});

Deno.test("incr - errors on overflow", async () => {
  const redis = await makeRedis();
  const key = rkey("incr_overflow");

  await redis.set(key, "9223372036854775807");
  await assertRejects(() => redis.incr(key), ErrorReplyError);

  redis.close();
});
