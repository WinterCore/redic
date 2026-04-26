# Redic

<h4 align="center">Redis server/client implementation in C from scratch with no dependencies</h4>

<br />
<br />

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
- [x] `PING [message]`
- [x] `SET key value [NX|XX] [GET] [EX|PX|EXAT|PXAT|KEEPTTL]`
- [x] `GET key`
- [x] `INFO` (replication section)
- [x] TTL stored per key with lazy expiry on access
- [ ] `DEL key [key ...]` — delete one or more keys, returns the count of keys removed
- [ ] `EXISTS key [key ...]` — returns how many of the given keys exist
- [ ] `EXPIRE key seconds` — set a timeout on a key after which it is automatically deleted
- [ ] `TTL key` — return the remaining time to live of a key in seconds
- [ ] `INCR` / `DECR` / `INCRBY` / `DECRBY` — atomically increment or decrement an integer stored as a string
- [ ] `APPEND key value` — append a string to an existing value, creating the key if it doesn't exist
- [ ] `MGET key [key ...]` — get the values of multiple keys in a single command
- [ ] `MSET key value [key value ...]` — set multiple keys in a single command
- [ ] `KEYS pattern` — return all keys matching a glob-style pattern (e.g. `KEYS user:*`)
- [ ] `TYPE key` — return the data type stored at a key (string, list, hash, set)
- [ ] `RENAME key newkey` — atomically rename a key
- [ ] Active expiry (background task that periodically sweeps expired keys)
- [ ] Inline command support (plain-text commands via telnet/netcat)
- [ ] Pipelining (handle multiple commands in a single read)
- [ ] Lists (`LPUSH`, `RPUSH`, `LPOP`, `RPOP`, `LRANGE`) — ordered collection with O(1) head/tail operations
- [ ] Hashes (`HSET`, `HGET`, `HGETALL`, `HDEL`) — map of string fields to string values stored under one key
- [ ] Sets (`SADD`, `SREM`, `SMEMBERS`, `SISMEMBER`) — unordered collection of unique strings
- [ ] Replica handshake with master (`PING` → `REPLCONF` → `PSYNC`)
- [ ] Master propagates write commands to replicas
- [ ] AOF persistence — append each write command to a file and replay it on startup to restore state

### References
- [Redis serialization protocol specification
](https://redis.io/docs/latest/develop/reference/protocol-spec/)
- [C Hashmap Implementation](https://github.com/petewarden/c_hashmap/tree/master)
- [Tsoding's Arena allocator implementation](https://github.com/tsoding/arena)
