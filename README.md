# Redic

<h4 align="center">Redis server/client implementation in C from scratch with no dependencies</h4>

<br />

<p align="center">
  <img src="redic-logo.png" alt="description" width="400">
</p>

<br />

## How it works

Redic is a from-scratch Redis-compatible server written in C. It speaks the [RESP protocol](https://redis.io/docs/latest/develop/reference/protocol-spec/), accepts concurrent client connections (one thread per client), parses incoming commands against declarative argument schemas, and dispatches them to a single-threaded data store.

### Actor pattern & serializability

The data layer uses an actor pattern to guarantee serializable access without locks. All reads and writes go through a message-passing pipeline rather than touching shared memory directly:

- **Sewer** — a channel backed by a ring buffer that passes poop (messages) from client threads down to the data store. Client threads push an operation in and block until they get a response back through a per-request reply channel.
- **Septic Tank** — where all the poop ends up. A single-threaded actor that drains the sewer one message at a time, executes the operation against the in-memory hashmap, and flushes the result back to the caller.
- **Potty** — a dedicated persistence actor for write mutations (`SET`, `DEL`, `EXPIRE`). Successful mutations are forwarded from the septic tank to potty in the same operation order so they can be written as an append-only stream for recovery.

Because the septic tank is the only thread that ever touches the data, there are no data races and no mutexes needed.

### Potty (AOF path)

Potty is Redic's append-only persistence path (Redis-style AOF direction). Everything lands in a single append-only file, `data.aof`, in the server's working directory.

**Routing.** The septic tank executes a command first, then forwards it to potty only if it actually mutated state — so a failed `NX`, a `WRONGTYPE`, or a `DEL` on a missing key writes nothing. Mutations travel as `SepticTankMutation` payloads over potty's own sewer channel. Because both the in-memory write and the potty hand-off are driven by the single septic tank actor, disk order always matches memory order.

Only three record types ever reach disk. `INCR`/`DECR`/`INCRBY`/`DECRBY` resolve to a `SET` of the computed value, and an `EXPIRE` with a timestamp already in the past resolves to a `DEL`, so the log stores outcomes rather than the commands that produced them — replay never has to re-run any arithmetic or re-evaluate a condition.

**Buffering and flush triggers.** The potty actor drains its sewer with a 1-second timeout rather than blocking forever, so it still wakes up to evaluate time-based flushes while idle. Incoming mutations are cloned into an arena-backed buffer (`waste`), which is flushed when either threshold trips:

- **Size** — 500 buffered mutations
- **Time** — 5 seconds since the last flush

**Flush handoff.** A flush doesn't block the actor. The current `{waste, arena}` pair is packaged as a `FlushJob` and pushed onto a queue, and a fresh buffer is installed immediately so incoming writes keep flowing. A detached flusher thread is spawned if one isn't already running; it drains the queue FIFO, appending each job to `data.aof` and calling `F_FULLFSYNC` (macOS) or `fsync` (elsewhere) before closing. When the queue empties, the flusher re-checks it under the mutex before clearing `flusher_running` and exiting, which closes the race where the actor enqueues a job while the flusher is winding down.

**On-disk format.** Records are raw binary, written back to back with no file header and no framing between them. Each is a 1-byte type tag followed by its fields; strings are a `uint64` length followed by that many raw bytes (binary-safe, no terminator), and integers are native-endian:

| Record | Payload |
|---|---|
| `SET` | `int64` expiry (unix ms, `-1` = no expiry), key, value |
| `DEL` | key |
| `EXPIRE` | key, `int64` expires-at (unix ms) |

**Durability caveats.** Up to one flush interval of writes can be lost on crash — the Redis `everysec` tradeoff. There are no checksums or record-length prefixes, and writes append directly to the live file rather than going through a temp-file-and-rename, so a crash mid-write leaves a partial record at the tail that replay will have to detect and discard. Clean shutdown doesn't flush the pending buffer yet either.

Current status: the write path is complete end to end. Startup replay is not implemented — nothing reads `data.aof` back yet.

## Running locally
- Make sure you have `make` and any **C** compiler installed on your system.
- Run `git clone git@github.com:WinterCore/redic.git && cd redic`
- To build the project, simply run `make`
- And finally, to run the server `./Redic` (it will run on port 6969 by default)

## Interacting with the server
- You can use any Redis cli or you can just use `redis-cli` which comes bundled with Redis
- Don't forget to specify the port of Redic when running commands, eg: `redis-cli -p 6969 SET foo bar`

## TODO

- [x] TCP server with configurable port
- [x] Concurrent client connections (one thread per client)
- [x] RESP protocol parser (simple string, bulk string, array, integer, error, null)
- [x] RESP serializer
- [x] Actor pattern for data layer — single-threaded data store (septic tank) with lock-free reads/writes via message-passing (sewer channels + ring buffer)
- [x] TTL stored per key with lazy expiry on access
- [ ] Inline command support (plain-text commands via telnet/netcat)
- [ ] Pipelining (handle multiple commands in a single read)
- [ ] Active expiry — background task that periodically sweeps expired keys
- [ ] Commands
  - [x] `PING [message]`
  - [x] `SET key value [NX|XX] [GET] [EX|PX|EXAT|PXAT|KEEPTTL]`
  - [x] `GET key`
  - [x] `DEL key [key ...]` (single key for now cuz command parser doesn't support variadic args)
  - [x] `TTL key`
  - [x] `INFO`
  - [x] `EXISTS key [key ...]` (single key for now)
  - [x] `EXPIRE key seconds`
  - [x] `INCR` / `DECR` / `INCRBY` / `DECRBY`
  - [ ] `APPEND key value`
  - [ ] `MGET key [key ...]` / `MSET key value [key value ...]`
  - [ ] `KEYS pattern`
  - [ ] `TYPE key`
  - [ ] `RENAME key newkey`
  - [ ] Lists (`LPUSH`, `RPUSH`, `LPOP`, `RPOP`, `LRANGE`)
  - [ ] Hashes (`HSET`, `HGET`, `HGETALL`, `HDEL`)
  - [ ] Sets (`SADD`, `SREM`, `SMEMBERS`, `SISMEMBER`)
- [ ] AOF persistence — mutation routing, batched flushing, and fsync'd appends to `data.aof` are done; still need startup replay, torn-tail recovery, flush-on-shutdown, and compaction
- [ ] Replication — replica handshake (`PING` → `REPLCONF` → `PSYNC`), full resync on connect, partial resync via replication backlog after reconnect
- [ ] Transactions — `MULTI` / `EXEC` / `DISCARD` with `WATCH` for optimistic locking
- [ ] Pub/Sub — `SUBSCRIBE` / `PUBLISH` / `UNSUBSCRIBE` with fan-out to blocking subscribers

### References
- [Redis serialization protocol specification
](https://redis.io/docs/latest/develop/reference/protocol-spec/)
- [C Hashmap Implementation](https://github.com/petewarden/c_hashmap/tree/master)
- [Tsoding's Arena allocator implementation](https://github.com/tsoding/arena)
