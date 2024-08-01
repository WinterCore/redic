#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include "./hashmap.h"
#include "./aids.h"

/**
 * Global Redic server object.
 *
 * It contains things like config, data and the tcp connection
 */
typedef struct Server {
    Arena *arena;

    pthread_mutex_t data_lock;
    map_t data_map;
} Server;

Server create_server_instance();

typedef struct ClientSocketHandlerInput {
    Server *server;
    int socket_fd;
} ClientSocketHandlerInput;

void *handle_client_socket(void *handler_input);

typedef struct MapEntry {
    time_t expire_at;
    bool expires;

    // TODO: Implement different data types
    void *value;
} MapEntry;

#endif
