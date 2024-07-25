#include <stdint.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "./resp/resp.h"
#include "./command/command.h"
#include "arena.h"
#include "server.h"
#include "./aids.h"


#define CONNECTION_QUEUE_SIZE 1

/*
void test() {
    char *input = "*3\r\n$5\r\nhello\r\n*1\r\n+world\r\n";

    RESPValue value = {};
    
    RESPParseResult result = resp_parse_input(input, &value);   
    DEBUG_PRINT("RESULT: %d %zu", result.code, result.pos);

    print_value(&value);

    char *input = "*2\r\n$5\r\nhello\r\n$5\r\nworld\r\n";

    RESPValue value = {};
    
    RESPParseResult result = resp_parse_input(input, &value);   

    char buff[1000];

    resp_serialize_value(&buff[0], &value);
    
    printf("%d %d %s", buff[1] == '\r', buff[2] == '\n', buff);

    // DEBUG_PRINT("VALUE KIND: %d", value.kind == '_');
}
*/

int main() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    uint16_t port = 6969;

    /*
    Arena *arena = arena_create();
    
    CommandArgDefinition *arg_defs = arena_alloc(arena, sizeof(CommandArgDefinition) * 0);
    arg_defs[0].type = ARG_TYPE_STRING;
    arg_defs[0].is_optional = false;
    arg_defs[1].type = ARG_TYPE_STRING;
    arg_defs[1].is_optional = false;
    DEBUG_PRINT("PTR %zu", sizeof(CommandArgDefinition *));
    DEBUG_PRINT("PTR %p", arg_defs);
    DEBUG_PRINT("PTR %p", &arg_defs[0]);
    DEBUG_PRINT("PTR %p", &arg_defs[1]);

    RESPBulkString **input_args = arena_alloc(arena, sizeof(RESPBulkString *) * 0);

    CommandArg *command_args = arena_alloc(arena, sizeof(CommandArg) * 0);

    parse_command_arguments(
        arena,
        2,
        &arg_defs,
        0,
        input_args,
        command_args
    );

    UNIMPLEMENTED("TERMINATE %s", "");
    */

    if (socket_fd < 0) {
        PANIC("Failed to create server socket");
    }
    
    int optval = 1;
    setsockopt(
        socket_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        (const void *) &optval,
        sizeof(int)
    );

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int bind_result = bind(
        socket_fd,
        (struct sockaddr *) &server_addr,
        sizeof(server_addr)
    );

    if (bind_result < 0) {
        PANIC("Failed to bind socket");
    }

    int listen_result = listen(socket_fd, CONNECTION_QUEUE_SIZE);

    printf("Server is up and running on port %hu\n\n", port);
    fflush(stdout);

    if (listen_result < 0) {
        PANIC("Failed to listen");
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    Server server = create_server_instance();

    while (1) {
        int client_fd = accept(
            socket_fd,
            (struct sockaddr *) &client_addr,
            &client_addr_len 
        );

        if (client_fd < 0) {
            // TODO: Don't actually exit the process here
            PANIC("Failed to accept client socket");
        }
        
        struct hostent *hostp = gethostbyaddr(
            (const char *) &client_addr.sin_addr.s_addr,
            sizeof(client_addr.sin_addr.s_addr),
            AF_INET
        );

        if (hostp == NULL) {
            PANIC("Failed to get host info");
        }

        char *host_addr = inet_ntoa(client_addr.sin_addr);

        if (host_addr == NULL) {
            PANIC("Failed to serialize client address");
        }

        DEBUG_PRINT("server established connection with %s (%s) %d\n", hostp->h_name, host_addr, client_addr.sin_port);

        ClientSocketHandlerInput *handler_input = malloc(sizeof(ClientSocketHandlerInput));
        // TODO: Check for malloc errors
        handler_input->socket_fd = client_fd;
        handler_input->server = &server;
        
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        pthread_create(&tid, &attr, handle_client_socket, handler_input);
        pthread_attr_destroy(&attr);

        DEBUG_PRINT("Spawned thread %s", "");
    }

    return 0;
}
