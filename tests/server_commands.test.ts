import { connect } from "@db/redis";
import { assertEquals, assertExists, assertStringIncludes } from "@std/assert";

async function makeRedis() {
  return await connect({ hostname: "127.0.0.1", port: 6969 });
}

Deno.test("ping - returns PONG", async () => {
  const redis = await makeRedis();

  assertEquals(await redis.ping(), "PONG");

  redis.close();
});

Deno.test("info - returns replication info", async () => {
  const redis = await makeRedis();

  const info = await redis.info();
  assertExists(info);
  assertStringIncludes(info, "role:master");

  redis.close();
});
