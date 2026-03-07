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


## Bugs

- **Shared read/write buffer** — the same 4096-byte buffer is used for both reading the request and serializing the response, meaning the response overwrites the input before parsing is done
- **No partial read handling** — a single `read()` call is assumed to contain a complete command; TCP can deliver data in chunks so large commands split across packets will fail to parse
- **Buffer overflow on response** — `resp_serialize_value` writes into the fixed 4096-byte buffer with no length limit, a large enough value will overflow it
- **Null array crashes the parser** — `*-1\r\n` is a valid RESP null array; `strtoull("-1")` wraps to a huge number causing the parser to loop trying to read billions of elements
- **Commands are case-sensitive** — command matching uses `strcmp` so `set foo bar` fails; Redis commands are case-insensitive
- **`gethostbyaddr` crashes the server** — a failed reverse DNS lookup on a new connection calls `PANIC` and kills the entire process, taking down all connected clients
- **`SET` uses `strlen` instead of bulk string length** — binary values containing null bytes are silently truncated; the length from the parsed `RESPBulkString` should be used instead
- **`ARG_TYPE_DOUBLE` truncates to integer** — `strtod()` result is stored into an `int64_t *`, silently truncating any floating point value

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
