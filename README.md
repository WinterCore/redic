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
- **Potty** — a dedicated persistence actor for write mutations (`SET`, `DEL`). Successful mutations are forwarded from the septic tank to potty in the same operation order so they can be written as an append-only stream for recovery.

Because the septic tank is the only thread that ever touches the data, there are no data races and no mutexes needed.

### Potty (AOF path)

Potty is Redic's append-only persistence path (Redis-style AOF direction):

- The septic tank executes commands first, then forwards only successful mutations to potty.
- Potty receives mutation payloads (`SepticTankMutation`) through its own sewer channel.
- This keeps persistence ordering consistent with in-memory write ordering because both are driven by the single septic tank actor.

Current status: potty scaffolding and mutation routing are implemented; on-disk flush/replay logic is still in progress.

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
  - [x] `DEL key [key ...]`
  - [x] `TTL key`
  - [x] `INFO`
  - [ ] `EXISTS key [key ...]`
  - [ ] `EXPIRE key seconds`
  - [ ] `INCR` / `DECR` / `INCRBY` / `DECRBY`
  - [ ] `APPEND key value`
  - [ ] `MGET key [key ...]` / `MSET key value [key value ...]`
  - [ ] `KEYS pattern`
  - [ ] `TYPE key`
  - [ ] `RENAME key newkey`
  - [ ] Lists (`LPUSH`, `RPUSH`, `LPOP`, `RPOP`, `LRANGE`)
  - [ ] Hashes (`HSET`, `HGET`, `HGETALL`, `HDEL`)
  - [ ] Sets (`SADD`, `SREM`, `SMEMBERS`, `SISMEMBER`)
- [-] AOF persistence — potty actor/mutation routing exists; still need durable flush format, fsync policy, startup replay, and compaction
- [ ] Replication — replica handshake (`PING` → `REPLCONF` → `PSYNC`), full resync on connect, partial resync via replication backlog after reconnect
- [ ] Transactions — `MULTI` / `EXEC` / `DISCARD` with `WATCH` for optimistic locking
- [ ] Pub/Sub — `SUBSCRIBE` / `PUBLISH` / `UNSUBSCRIBE` with fan-out to blocking subscribers

### References
- [Redis serialization protocol specification
](https://redis.io/docs/latest/develop/reference/protocol-spec/)
- [C Hashmap Implementation](https://github.com/petewarden/c_hashmap/tree/master)
- [Tsoding's Arena allocator implementation](https://github.com/tsoding/arena)
