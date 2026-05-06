// test.ts
import { connect } from "jsr:@db/redis@0.35.6";
import { assertEquals, assertExists } from "jsr:@std/assert@1.0.13";

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

Deno.test.ignore("set EX - key expires after seconds", async () => {
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

Deno.test.ignore("set PX - key expires after milliseconds", async () => {
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

Deno.test.ignore("set KEEPTTL - preserves existing TTL", async () => {
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
