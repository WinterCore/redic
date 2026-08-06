# Redic

<h4 align="center">Redis server/client implementation in C from scratch with no dependencies</h4>

<br />

<p align="center">
  <img src="redic-logo.png" alt="description" width="400">
</p>

<br />

## Running locally

- Make sure you have `make` and any **C** compiler installed on your system.
- Run `git clone git@github.com:WinterCore/redic.git && cd redic`
- To build the project, simply run `make`
- And finally, to run the server `./Redic` (it will run on port 6969 by default, use `--port 7000` to change it)

## Interacting with the server

- You can use any Redis cli or you can just use `redis-cli` which comes bundled with Redis
- Don't forget to specify the port of Redic when running commands, eg: `redis-cli -p 6969 SET foo bar`

## Running the tests

- The suite needs [Deno](https://deno.com/). It builds and launches its own server, so don't start one first.
- Run `deno task test`

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
