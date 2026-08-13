import { ErrorReplyError } from "@db/redis";
import { assertEquals, assertGreater, assertLess } from "@std/assert";
import { afterAll, beforeAll, describe, it } from "@std/testing/bdd";
import {
  acrossRestart,
  aofSize,
  connectTo,
  delay,
  restartRedic,
  type RunningRedic,
  startRedic,
  stopRedic,
  stopRedicAndClean,
  waitForAofFlush,
} from "./helpers/redic.ts";

function rkey(prefix: string) {
  return `${prefix}_${crypto.randomUUID().replace(/-/g, "").slice(0, 8)}`;
}

// src/potty/potty.c. Anything that has to outlast a flush window waits this
// long plus a margin.
const FLUSH_TIMEOUT_MS = 1000;

describe("aof", () => {
  describe("the log itself", () => {
    let server: RunningRedic;

    beforeAll(async () => {
      server = await startRedic();
    });

    afterAll(async () => {
      await stopRedicAndClean(server);
    });

    it("writes reach the aof", async () => {
      const redis = await connectTo(server);
      const before = await aofSize(`${server.dir}/data.aof`);

      await redis.set(rkey("aof"), "value");
      redis.close();

      const after = await waitForAofFlush(server.dir, before + 1);
      assertGreater(after, before, "the flusher should have appended the SET");
    });

    // The point of resolving operations into mutations before they are logged:
    // a command that changes nothing is not a fact, so it has nothing to say.
    it("commands that change nothing append nothing", async () => {
      const redis = await connectTo(server);
      const existing = rkey("noop_existing");

      await redis.set(existing, "untouched");

      const settled = await waitForAofFlush(
        server.dir,
        (await aofSize(`${server.dir}/data.aof`)) + 1,
      );

      // None of these resolve to a mutation
      assertEquals(await redis.get(rkey("noop_missing")), null);
      assertEquals(await redis.del(rkey("noop_missing")), 0);
      assertEquals(await redis.expire(rkey("noop_missing"), 100), 0);
      assertEquals(await redis.exists(existing), 1);
      assertEquals(await redis.ttl(existing), -1);
      assertEquals(await redis.set(existing, "nope", { nx: true }), null);
      assertEquals(await redis.set(rkey("noop_xx"), "nope", { xx: true }), null);

      redis.close();

      // Two flush windows: one to carry a mutation out if there were one, and
      // one to make sure we did not simply measure too early
      await delay(FLUSH_TIMEOUT_MS * 2);

      assertEquals(
        await aofSize(`${server.dir}/data.aof`),
        settled,
        "no-op commands must not grow the log",
      );
    });
  });

  describe("replay", () => {
    it("replays the basic write paths", async () => {
      await acrossRestart(async (redis) => {
        await redis.set("plain", "hello");
        await redis.set("withttl", "world", { ex: 100 });

        await redis.set("gone", "doomed");
        await redis.del("gone");

        // Last write wins, not first
        await redis.set("overwritten", "v1");
        await redis.set("overwritten", "v2");

        // ...including a resurrection after a delete
        await redis.set("resurrected", "v1");
        await redis.del("resurrected");
        await redis.set("resurrected", "v2");
      }, async (redis) => {
        assertEquals(await redis.get("plain"), "hello");
        assertEquals(await redis.get("withttl"), "world");
        assertGreater(await redis.ttl("withttl"), 0, "TTL should survive");
        assertEquals(await redis.get("gone"), null, "DEL should replay");
        assertEquals(await redis.get("overwritten"), "v2");
        assertEquals(await redis.get("resurrected"), "v2");
      });
    });

    // NX/XX/GET are intent, resolved away before anything is logged. If they
    // ever leaked into the file these would come back with the wrong value,
    // because replay applies every mutation unconditionally.
    it("replays conditional writes by their outcome, not their condition", async () => {
      await acrossRestart(async (redis) => {
        // NX that succeeded is a write like any other
        await redis.set("nx_taken", "first", { nx: true });

        // NX that failed changed nothing, so the original must stand
        await redis.set("nx_rejected", "original");
        await redis.set("nx_rejected", "rejected", { nx: true });

        // XX that succeeded
        await redis.set("xx_taken", "original");
        await redis.set("xx_taken", "updated", { xx: true });

        // XX against a missing key changed nothing
        await redis.set("xx_rejected", "rejected", { xx: true });

        // GET is a read bolted onto a write; the write still counts
        await redis.set("with_get", "v1");
        await redis.set("with_get", "v2", { get: true });
      }, async (redis) => {
        assertEquals(await redis.get("nx_taken"), "first");
        assertEquals(await redis.get("nx_rejected"), "original");
        assertEquals(await redis.get("xx_taken"), "updated");
        assertEquals(await redis.get("xx_rejected"), null);
        assertEquals(await redis.get("with_get"), "v2");
      });
    });

    // Expiries are logged as absolute unix ms, so a restart must not extend
    // them and time served while the server was down still counts.
    it("replays expiries as absolute deadlines", async () => {
      await acrossRestart(async (redis) => {
        await redis.set("far_future", "v", { ex: 1000 });
        await redis.set("expired_before_shutdown", "v", { px: 150 });

        await redis.set("expire_cmd", "v");
        await redis.expire("expire_cmd", 1000);

        await redis.set("keep_ttl", "v1", { ex: 1000 });
        await redis.set("keep_ttl", "v2", { keepttl: true });

        // Outlive the short PX above so the key is already dead on disk
        await delay(300);
      }, async (redis) => {
        const ttl = await redis.ttl("far_future");
        assertGreater(ttl, 0, "TTL should survive the restart");
        assertLess(ttl, 1001, "TTL should not be extended by the restart");

        assertEquals(
          await redis.get("expired_before_shutdown"),
          null,
          "a key whose deadline passed before shutdown must stay expired",
        );

        assertGreater(await redis.ttl("expire_cmd"), 0, "EXPIRE should replay");

        assertEquals(await redis.get("keep_ttl"), "v2");
        assertGreater(
          await redis.ttl("keep_ttl"),
          0,
          "KEEPTTL should have carried the old deadline into the log",
        );
      });
    });

    // An EXPIRE with a deadline already in the past is resolved into a DEL
    // before it is logged, so replay never has to reason about wall-clock.
    it("replays a past-dated EXPIRE as a delete", async () => {
      await acrossRestart(async (redis) => {
        await redis.set("expire_past", "v");
        await redis.sendCommand("EXPIRE", ["expire_past", -1]);

        await redis.set("survivor", "v");
      }, async (redis) => {
        assertEquals(await redis.get("expire_past"), null);
        assertEquals(await redis.get("survivor"), "v", "sanity check");
      });
    });

    // INCRBY and friends resolve into a SET of the computed value, so the log
    // never contains a read-modify-write that replay would have to redo.
    it("replays counters as their resolved value", async () => {
      await acrossRestart(async (redis) => {
        await redis.incr("incr");
        await redis.incr("incr");

        await redis.decr("decr");

        await redis.incrby("incrby", 7);
        await redis.incrby("incrby", 35);

        await redis.decrby("decrby", 5);

        await redis.set("counter_ttl", "5", { ex: 1000 });
        await redis.incr("counter_ttl");

        // A rejected INCR must leave the stored value alone, on disk too
        await redis.set("not_a_number", "abc");
        await redis.incr("not_a_number").catch((error: unknown) => {
          if (!(error instanceof ErrorReplyError)) throw error;
        });

        await redis.set("at_the_ceiling", "9223372036854775807");
        await redis.incr("at_the_ceiling").catch((error: unknown) => {
          if (!(error instanceof ErrorReplyError)) throw error;
        });
      }, async (redis) => {
        assertEquals(await redis.get("incr"), "2");
        assertEquals(await redis.get("decr"), "-1");
        assertEquals(await redis.get("incrby"), "42");
        assertEquals(await redis.get("decrby"), "-5");

        assertEquals(await redis.get("counter_ttl"), "6");
        assertGreater(
          await redis.ttl("counter_ttl"),
          0,
          "an incremented key should keep its deadline through a restart",
        );

        assertEquals(await redis.get("not_a_number"), "abc");
        assertEquals(await redis.get("at_the_ceiling"), "9223372036854775807");
      });
    });

    // Keys and values are length-prefixed in the file rather than delimited,
    // so none of these should need escaping to survive.
    it("replays awkward keys and values byte for byte", async () => {
      // Kept under the server's 4096-byte read buffer: a command that spans
      // two reads is a known gap in the RESP layer (src/server.c:34), not
      // something the log has any say in.
      const long = "x".repeat(3000);

      const cases: [string, string][] = [
        ["key with spaces", "value with spaces"],
        ["newlines", "line1\r\nline2\nline3"],
        ["crlf_only", "\r\n"],
        ["unicode", "مرحبا 🎉 café"],
        ["embedded_nul", "before\u0000after"],
        ["مفتاح_عربي", "قيمة"],
        ["long", long],
        ["resp_lookalike", "*2\r\n$3\r\nSET\r\n"],
      ];

      await acrossRestart(async (redis) => {
        for (const [key, value] of cases) {
          await redis.set(key, value);
        }
      }, async (redis) => {
        for (const [key, value] of cases) {
          assertEquals(await redis.get(key), value, `${key} did not round-trip`);
        }
      });
    });

    // Split out from the case above because it currently takes the server down
    // with it: fwrite(ptr, 0, 1) returns 0, and potty_serializer.c:56 reads
    // that as a write failure and panics mid-flush. Grouping it with the other
    // payloads would hide whether any of them work.
    it("replays an empty value", async () => {
      await acrossRestart(async (redis) => {
        await redis.set("empty_value", "");
        await redis.set("written_after_the_empty_one", "still here");
      }, async (redis) => {
        assertEquals(await redis.get("empty_value"), "");
        assertEquals(
          await redis.get("written_after_the_empty_one"),
          "still here",
          "an empty value must not truncate the log",
        );
      });
    });

    // 600 crosses FLUSH_ITEMS_THRESHOLD (500), so this spans more than one
    // flush job and proves the batches are not stepping on each other.
    it("replays a batch larger than the flush threshold", async () => {
      const count = 600;

      await acrossRestart(async (redis) => {
        for (let i = 0; i < count; i += 1) {
          await redis.set(`bulk_${i}`, `value_${i}`);
        }
      }, async (redis) => {
        for (let i = 0; i < count; i += 1) {
          assertEquals(
            await redis.get(`bulk_${i}`),
            `value_${i}`,
            `bulk_${i} went missing`,
          );
        }
      });
    });
  });

  describe("across several restarts", () => {
    it("keeps appending rather than rewriting", async () => {
      let server = await startRedic();

      try {
        const first = await connectTo(server);
        await first.set("era1", "one");
        first.close();

        server = await restartRedic(server, 0);
        const afterFirst = await aofSize(`${server.dir}/data.aof`);

        const second = await connectTo(server);
        assertEquals(await second.get("era1"), "one", "first era should replay");
        await second.set("era2", "two");
        second.close();

        server = await restartRedic(server, afterFirst);
        const afterSecond = await aofSize(`${server.dir}/data.aof`);

        assertGreater(
          afterSecond,
          afterFirst,
          "the second era should have been appended, not overwritten",
        );

        const third = await connectTo(server);

        // The interesting one: era1 came back from a replay that itself ran
        // with logging disabled, so it only survives if the original record is
        // still in the file.
        assertEquals(await third.get("era1"), "one", "first era should survive twice");
        assertEquals(await third.get("era2"), "two", "second era should replay");
        third.close();
      } finally {
        await stopRedicAndClean(server);
      }
    });

    it("does not re-log what it replayed", async () => {
      let server = await startRedic();

      try {
        const first = await connectTo(server);
        await first.set("stable", "value");
        first.close();

        server = await restartRedic(server, 0);
        const afterFirst = await aofSize(`${server.dir}/data.aof`);

        // Nothing written this time round, so a replay that logged what it
        // read would double the file
        await delay(FLUSH_TIMEOUT_MS * 2);
        await stopRedic(server);

        assertEquals(
          await aofSize(`${server.dir}/data.aof`),
          afterFirst,
          "replay must not append the records it just read",
        );

        server = await startRedic(server.dir);
        const second = await connectTo(server);
        assertEquals(await second.get("stable"), "value");
        second.close();
      } finally {
        await stopRedicAndClean(server);
      }
    });
  });
});
