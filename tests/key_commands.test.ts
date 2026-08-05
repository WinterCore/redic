import { connect, type Redis } from "@db/redis";
import { assertEquals, assertGreater, assertLessOrEqual } from "@std/assert";

async function makeRedis() {
  return await connect({ hostname: "127.0.0.1", port: 6969 });
}

function rkey(prefix: string) {
  return `${prefix}_${crypto.randomUUID().replace(/-/g, "").slice(0, 8)}`;
}

// The client's typed expire() has no mode argument, so the NX/XX/GT/LT
// conditions go through the raw command interface.
async function expireWithMode(
  redis: Redis,
  key: string,
  seconds: number,
  mode: "NX" | "XX" | "GT" | "LT",
): Promise<number> {
  return await redis.sendCommand("EXPIRE", [key, seconds, mode]) as number;
}

Deno.test("del - returns 1 when key exists", async () => {
  const redis = await makeRedis();
  const key = rkey("del");

  await redis.set(key, "value");
  const result = await redis.del(key);
  assertEquals(result, 1);

  redis.close();
});

Deno.test("del - returns 0 when key does not exist", async () => {
  const redis = await makeRedis();

  const result = await redis.del(rkey("missing"));
  assertEquals(result, 0);

  redis.close();
});

Deno.test("del - key is no longer accessible after deletion", async () => {
  const redis = await makeRedis();
  const key = rkey("del_get");

  await redis.set(key, "value");
  await redis.del(key);
  const value = await redis.get(key);
  assertEquals(value, null);

  redis.close();
});

Deno.test("ttl - returns -2 for missing key", async () => {
  const redis = await makeRedis();

  const result = await redis.ttl(rkey("missing"));
  assertEquals(result, -2);

  redis.close();
});

Deno.test("ttl - returns -1 for key with no expiry", async () => {
  const redis = await makeRedis();
  const key = rkey("ttl_no_expiry");

  await redis.set(key, "value");
  const result = await redis.ttl(key);
  assertEquals(result, -1);

  redis.close();
});

Deno.test("ttl - returns remaining seconds for key with expiry", async () => {
  const redis = await makeRedis();
  const key = rkey("ttl_expiry");

  await redis.set(key, "value", { ex: 10 });
  const result = await redis.ttl(key);
  assertGreater(result, 0);
  assertLessOrEqual(result, 10);

  redis.close();
});

Deno.test("ttl - returns -2 after key is deleted", async () => {
  const redis = await makeRedis();
  const key = rkey("ttl_del");

  await redis.set(key, "value", { ex: 10 });
  await redis.del(key);
  const result = await redis.ttl(key);
  assertEquals(result, -2);

  redis.close();
});

Deno.test("ttl - returns seconds when set with PX", async () => {
  const redis = await makeRedis();
  const key = rkey("ttl_px");

  await redis.set(key, "value", { px: 10000 });
  const result = await redis.ttl(key);
  assertGreater(result, 0);
  assertLessOrEqual(result, 10);

  redis.close();
});

Deno.test("exists - returns 1 for an existing key", async () => {
  const redis = await makeRedis();
  const key = rkey("exists");

  await redis.set(key, "value");
  assertEquals(await redis.exists(key), 1);

  redis.close();
});

Deno.test("exists - returns 0 for a missing key", async () => {
  const redis = await makeRedis();

  assertEquals(await redis.exists(rkey("exists_missing")), 0);

  redis.close();
});

Deno.test("exists - returns 0 for an expired key", async () => {
  const redis = await makeRedis();
  const key = rkey("exists_expired");

  await redis.set(key, "value", { px: 100 });
  await new Promise((r) => setTimeout(r, 200));
  assertEquals(await redis.exists(key), 0);

  redis.close();
});

Deno.test("expire - sets a TTL on an existing key", async () => {
  const redis = await makeRedis();
  const key = rkey("expire");

  await redis.set(key, "value");
  assertEquals(await redis.expire(key, 100), 1);

  const ttl = await redis.ttl(key);
  assertGreater(ttl, 0);
  assertLessOrEqual(ttl, 100);

  redis.close();
});

Deno.test("expire - returns 0 for a missing key", async () => {
  const redis = await makeRedis();

  assertEquals(await redis.expire(rkey("expire_missing"), 100), 0);

  redis.close();
});

Deno.test("expire - key is gone after the timeout elapses", async () => {
  const redis = await makeRedis();
  const key = rkey("expire_elapse");

  await redis.set(key, "value");
  await redis.expire(key, 1);
  await new Promise((r) => setTimeout(r, 1100));

  assertEquals(await redis.get(key), null);

  redis.close();
});

Deno.test("expire - a past timeout deletes the key immediately", async () => {
  const redis = await makeRedis();
  const key = rkey("expire_past");

  await redis.set(key, "value");
  assertEquals(await redis.expire(key, -1), 1, "a past expiry should report the timeout was set");
  assertEquals(await redis.get(key), null, "key should be gone after a past expiry");

  redis.close();
});

Deno.test("expire NX - only sets when the key has no expiry", async () => {
  const redis = await makeRedis();
  const key = rkey("expire_nx");

  await redis.set(key, "value");
  assertEquals(await expireWithMode(redis, key, 100, "NX"), 1, "NX sets when there is no expiry");
  assertEquals(await expireWithMode(redis, key, 200, "NX"), 0, "NX is a no-op once an expiry exists");

  redis.close();
});

Deno.test("expire XX - only sets when the key already has an expiry", async () => {
  const redis = await makeRedis();
  const key = rkey("expire_xx");

  await redis.set(key, "value");
  assertEquals(await expireWithMode(redis, key, 100, "XX"), 0, "XX is a no-op with no existing expiry");

  await redis.expire(key, 100);
  assertEquals(await expireWithMode(redis, key, 200, "XX"), 1, "XX sets when an expiry already exists");

  redis.close();
});

Deno.test("expire GT - only sets when the new expiry is greater", async () => {
  const redis = await makeRedis();
  const key = rkey("expire_gt");

  await redis.set(key, "value");
  await redis.expire(key, 100);

  assertEquals(await expireWithMode(redis, key, 50, "GT"), 0, "GT rejects a smaller expiry");
  assertEquals(await expireWithMode(redis, key, 500, "GT"), 1, "GT accepts a larger expiry");
  assertGreater(await redis.ttl(key), 100);

  redis.close();
});

Deno.test("expire LT - only sets when the new expiry is smaller", async () => {
  const redis = await makeRedis();
  const key = rkey("expire_lt");

  await redis.set(key, "value");
  await redis.expire(key, 100);

  assertEquals(await expireWithMode(redis, key, 500, "LT"), 0, "LT rejects a larger expiry");
  assertEquals(await expireWithMode(redis, key, 20, "LT"), 1, "LT accepts a smaller expiry");
  assertLessOrEqual(await redis.ttl(key), 20);

  redis.close();
});
