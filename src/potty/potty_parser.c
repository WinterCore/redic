#include <stdint.h>
#include <stdio.h>

#include "potty_parser.h"

static int fread_byte(FILE *file, uint8_t *buffer) {
    uint64_t items = fread(buffer, 1, 1, file);
    
    return items;
}

PottyParser potty_parser_create(FILE *file) {
    PottyParser parser = {
        .file = file,
        .eof_reached = false,
        .position = 0,
    };

    return parser;
}

PottyParserReadResult potty_parser_read(
    Arena *arena,
    FILE *file,
    SepticTankMutation *out_mutation
) {
    SepticTankOperationType type;

    // Read type
    int result = fread_byte(file, (uint8_t *) &type);

    if (feof(file)) {
        return POTTY_PARSER_EOF;
    }

    if (ferror(file)) {
        return POTTY_PARSER_READ_ERROR;
    }
    
    switch (type) {
        case SEPTIC_TANK_GET:
        case SEPTIC_TANK_TTL:
        case SEPTIC_TANK_EXISTS:
        case SEPTIC_TANK_INCRBY:
            UNREACHABLE();
            break;

        case SEPTIC_TANK_SET:
            UNIMPLEMENTED("");
        case SEPTIC_TANK_DEL:
            UNIMPLEMENTED("");
        case SEPTIC_TANK_EXPIRE:
            UNIMPLEMENTED("");
    }
}
