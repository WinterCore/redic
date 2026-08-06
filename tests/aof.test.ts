import { assertEquals, assertGreater } from "@std/assert";
import { afterAll, beforeAll, describe, it } from "@std/testing/bdd";
import {
  aofSize,
  connectTo,
  type RunningRedic,
  startRedic,
  stopRedic,
  waitForAofFlush,
} from "./helpers/redic.ts";

function rkey(prefix: string) {
  return `${prefix}_${crypto.randomUUID().replace(/-/g, "").slice(0, 8)}`;
}

describe("aof", () => {
  let server: RunningRedic;

  beforeAll(async () => {
    server = await startRedic();
  });

  afterAll(async () => {
    await stopRedic(server);
    await Deno.remove(server.dir, { recursive: true });
  });

  it("writes reach the aof", async () => {
    const redis = await connectTo(server);
    const before = await aofSize(`${server.dir}/data.aof`);

    await redis.set(rkey("aof"), "value");
    redis.close();

    const after = await waitForAofFlush(server.dir, before + 1);
    assertGreater(after, before, "the flusher should have appended the SET");
  });

  // Restarts the process on the same directory, so the second server opens the
  // AOF the first one wrote. Every mutating command we currently implement is
  // exercised: DEL to prove deletes replay rather than being skipped, and
  // INCRBY to prove it lands on disk as the SET it resolves into.
  it("data survives a restart", async () => {
    const first = await connectTo(server);

    await first.set("plain", "hello");
    await first.set("withttl", "world", { ex: 100 });
    await first.set("gone", "doomed");
    await first.del("gone");
    await first.incrby("counter", 7);

    first.close();

    // No graceful shutdown yet, so the only way to get the tail of the log onto
    // disk is to outwait FLUSH_TIMEOUT (src/potty/potty.c). Once flush-on-exit
    // lands this can become a plain stop.
    await waitForAofFlush(server.dir);
    await stopRedic(server);

    server = await startRedic(server.dir);
    const second = await connectTo(server);

    assertEquals(await second.get("plain"), "hello", "SET should replay");
    assertEquals(await second.get("withttl"), "world", "SET with an expiry should replay");
    assertGreater(await second.ttl("withttl"), 0, "the replayed key should keep its TTL");
    assertEquals(await second.get("gone"), null, "DEL should replay");
    assertEquals(await second.get("counter"), "7", "INCRBY should replay as a SET");

    second.close();
  });
});
