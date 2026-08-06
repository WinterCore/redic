import { assertEquals, assertExists, assertStringIncludes } from "@std/assert";
import { afterAll, beforeAll, describe, it } from "@std/testing/bdd";
import {
  connectTo,
  type RunningRedic,
  startRedic,
  stopRedicAndClean,
} from "./helpers/redic.ts";

describe("server commands", () => {
  let server: RunningRedic;

  beforeAll(async () => {
    server = await startRedic();
  });

  afterAll(async () => {
    await stopRedicAndClean(server);
  });

  const makeRedis = () => connectTo(server);

  it("ping - returns PONG", async () => {
    const redis = await makeRedis();

    assertEquals(await redis.ping(), "PONG");

    redis.close();
  });

  it("info - returns replication info", async () => {
    const redis = await makeRedis();

    const info = await redis.info();
    assertExists(info);
    assertStringIncludes(info, "role:master");

    redis.close();
  });
});
