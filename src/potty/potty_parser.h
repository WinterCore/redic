#ifndef POTTY_PARSER_H
#define POTTY_PARSER_H

#include <stdio.h>
#include <stdbool.h>
#include "../arena.h"
#include "../septic_tank/septic_tank_operation.h"

typedef struct PottyParser {
    Arena *arena;

    FILE *file;
    bool eof_reached;
    int position;
} PottyParser;

PottyParser potty_parser_create(Arena *arena, FILE *file);


typedef enum PottyParserReadResult: uint8_t {
    POTTY_PARSER_EOF,
    POTTY_PARSER_UNEXPECTED_EOF,
    POTTY_PARSER_READ_ERROR,
} PottyParserReadResult;

PottyParserReadResult potty_parser_read(
    PottyParser *parser,
    SepticTankMutation *out_mutation
);

#endif
