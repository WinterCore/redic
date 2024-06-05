#ifndef COMMAND_H
#define COMMAND_H

#include "../resp/resp.h"
#include "../server.h"

#define REGISTER_COMMAND(CMD_NAME, ARGS, HANDLER_FN) \

enum COMMAND_ARG_TYPE {
    STRING,
    INTEGER,
    DOUBLE,
    KEY,
    // ...
};

typedef struct Option {
    bool is_present;
    void *value;
} Option;

typedef struct CommandDefinition {
    char *name;
} CommandDefinition;

RESPValue process_ping(Server *server);

#endif
