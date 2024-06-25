#ifndef SERVER_H
#define SERVER_H

#include "./hashmap.h"

/**
 * Global Redic server object.
 *
 * It contains things like config, data and the tcp connection
 */
typedef struct Server {
    map_t data_map;
} Server;

Server create_server_instance();

typedef struct ClientSocketHandlerInput {
    Server *server;
    int socket_fd;
} ClientSocketHandlerInput;

void *handle_client_socket(void *handler_input);

#endif
