#include <stdio.h>
#include <unistd.h>

#include "./arena.h"
#include "./server.h"
#include "./aids.h"
#include "./resp/resp.h"
#include "./command/command.h"
#include "hashmap.h"


Server create_server_instance() {
    Server server = {
        .data_map = hashmap_new(),
    };
    
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

        RESPValue response_value = process_command(arena, NULL, &value);

        size_t serialized_len = resp_serialize_value(buffer, &response_value);


        // TODO: Handle write error
        DEBUG_PRINT("%s", buffer);
        write(handler_input->socket_fd, buffer, serialized_len);

        arena_reset(arena);
    }

    free(handler_input);

    return NULL;
}
