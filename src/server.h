#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <arpa/inet.h>

#include "./hashmap.h"
#include "./aids.h"

typedef struct SocketInfo {
    struct in_addr ip;
    uint16_t port;
} SocketInfo;

/**
 * Global Redic server object.
 *
 * It contains things like config, data and the tcp connection
 */
typedef struct Server {
    Arena *arena;

    pthread_mutex_t data_lock;
    map_t data_map;
    
    /**
     * The master this replica is connected to (only applies to replicas)
     *
     * Option<SocketInfo>
     */
    Option maybe_master;

    /*
     * Check maybe_master before accessing these
     * TODO: Group master/replica only attributes in their own option to make
     * the data easier to access
     */
    char *master_replid;
    ssize_t master_repl_offset;

} Server;

Server create_server_instance();

typedef struct ClientSocketHandlerInput {
    Server *server;
    int socket_fd;
} ClientSocketHandlerInput;

void *handle_client_socket(void *handler_input);
bool parse_socket_info(char *input, SocketInfo *socket_info);

#endif
