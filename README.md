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


### References
- [Redis serialization protocol specification
](https://redis.io/docs/latest/develop/reference/protocol-spec/)
- [C Hashmap Implementation](https://github.com/petewarden/c_hashmap/tree/master)
- [Tsoding's Arena allocator implementation](https://github.com/tsoding/arena)
