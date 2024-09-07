#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "./arena.h"
#include "./server.h"
#include "./aids.h"
#include "./resp/resp.h"
#include "./command/command.h"
#include "./hashmap.h"


Server create_server_instance() {
    Server server = {
        .arena = arena_create(),
        .data_map = hashmap_new(),
        .maybe_master = (Option) { .is_present = false },
    };

    if (pthread_mutex_init(&server.data_lock, NULL) != 0) {
        PANIC("Failed to initialize data mutex");
    }
    
    return server;
}


void *handle_client_socket(void *_handler_input) {
    ClientSocketHandlerInput *handler_input = _handler_input;

    Arena *arena = arena_create();

    char buffer[4096];

    // Read from client forever
    while (1) {
        ssize_t bytes_read = read(handler_input->socket_fd, buffer, sizeof(buffer));

        if (bytes_read == 0) {
            DEBUG_PRINT("Connection was closed %s", "");

            break;
        }

        if (bytes_read < 0) {
            DEBUG_PRINT("An error occurred while reading! %ld", bytes_read);

            break;
        }

        DEBUG_PRINT("Read %ld bytes", bytes_read);

        RESPValue value = {0};

        RESPParseResult result = resp_parse_input(arena, buffer, &value);

        resp_print_parse_result(&result);
        resp_print_value(&value);

        RESPValue response_value = process_command(arena, handler_input->server, &value);

        size_t serialized_len = resp_serialize_value(buffer, &response_value);


        // TODO: Handle write error
        DEBUG_PRINT("%s", buffer);
        write(handler_input->socket_fd, buffer, serialized_len);

        arena_reset(arena);
    }

    free(handler_input);

    return NULL;
}


bool parse_socket_info(char *input, SocketInfo *socket_info) {
    char *rest = input;
    char *ipstr = strsep(&rest, " ");

    if (ipstr == NULL || rest == NULL) {
        return false;
    }

    if (inet_pton(AF_INET, ipstr, &socket_info->ip) != 1) {
        return false;
    }

    char *endptr;

    socket_info->port = strtol(rest, &endptr, 10);

    if (endptr == rest) {
        return false;
    }

    return true;
}
