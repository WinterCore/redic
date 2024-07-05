#include <pthread.h>
#include <string.h>

#include "../../hashmap.h"
#include "../command.h"

RESPValue process_set(Arena *arena, Server *server, CommandArg **args);

CommandDefinition SET_COMMAND = COMMAND(
    "SET",
    ((CommandArgDefinition []) {
        COMMAND_ARGUMENT("key", ARG_TYPE_KEY, false),
        COMMAND_ARGUMENT("value", ARG_TYPE_STRING, false)
    }),
    process_set
);

RESPValue process_set(Arena *arena, Server *server, CommandArg **args) {

    char *temp_key = args[0]->value;
    char *temp_value = args[1]->value;
    size_t key_len = strlen(temp_key);
    size_t value_len = strlen(temp_value);
    
    char *key = malloc(strlen(temp_key));
    char *value = malloc(strlen(temp_value));

    if (key == NULL || value == NULL) {
        return resp_create_simple_error_value(arena, "Failed to allocate memory");
    }


    memcpy(key, temp_key, key_len);
    memcpy(value, temp_value, value_len);

    pthread_mutex_lock(&server->data_lock);
    hashmap_put(server->data_map, key, value);
    pthread_mutex_unlock(&server->data_lock);

    return resp_create_simple_string_value(arena, "OK");
}
