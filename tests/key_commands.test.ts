import { connect } from "@db/redis";
import { assertEquals, assertGreater, assertLessOrEqual } from "@std/assert";

async function makeRedis() {
  return await connect({ hostname: "127.0.0.1", port: 6969 });
}

function rkey(prefix: string) {
  return `${prefix}_${crypto.randomUUID().replace(/-/g, "").slice(0, 8)}`;
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
