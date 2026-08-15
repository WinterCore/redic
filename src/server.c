#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "./arena.h"
#include "./server.h"
#include "./aids.h"
#include "./resp/resp.h"
#include "./command/command.h"
#include "sewer.h"

Server create_server_instance() {
    Server server = {
        .arena = arena_create(),
        .septic_tank_sewer = sewer_create(100),
        .potty_sewer = sewer_create(100),
        .maybe_master = (Option) { .is_present = false },
    };

    return server;
}


void *handle_client_socket(void *_handler_input) {
    ClientSocketHandlerInput *handler_input = _handler_input;

    Arena *arena = arena_create();

    char buffer[4096];

    // Read from client forever
    while (1) {
        // TODO: need to wire parser with the read command because a complete command
        // may not always arrive in a single read call
        ssize_t bytes_read = read(handler_input->socket_fd, buffer, sizeof(buffer));

        if (bytes_read == 0) {
            DEBUG_PRINTF("Connection was closed\n");

            break;
        }

        if (bytes_read < 0) {
            DEBUG_PRINTF("An error occurred while reading! %ld\n", bytes_read);

            break;
        }

        // DEBUG_PRINT("Read %ld bytes", bytes_read);

        RESPValue value = {0};

        RESPParseResult result = resp_parse_input(arena, buffer, &value);
        UNUSED(result);

        /*
        resp_print_parse_result(&result);
        resp_print_value(&value);
        */

        RESPValue response_value = process_command(arena, handler_input->server, &value);

        size_t serialized_len = resp_serialize_value(buffer, &response_value);


        // TODO: Handle write error
        // DEBUG_PRINT("%s", buffer);
        write(handler_input->socket_fd, buffer, serialized_len);

        arena_reset(arena);
    }

    arena_destroy(arena);
    free(handler_input);

    return NULL;
}


bool parse_socket_info(char *input, SocketInfo *socket_info) {
    char *space = strchr(input, ' ');

    if (space == NULL) {
        return false;
    }

    *space = '\0';
    char *ipstr = input;
    char *rest = space + 1;

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
