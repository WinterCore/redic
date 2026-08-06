#include <stdint.h>
#include <stdio.h>

#include "potty_parser.h"

#define handle_fread_result(file, result, fail_on_eof) \
    do { \
        if (ferror(file)) return POTTY_PARSER_READ_ERROR; \
        if (result < 1 && feof(file)) { \
            return fail_on_eof \
                ? POTTY_PARSER_UNEXPECTED_EOF \
                : POTTY_PARSER_EOF; \
        } \
    } while (0)

#define fread_byte(file, out, fail_on_eof) \
    do { \
        int result = fread(out, 1, 1, file); \
        handle_fread_result(file, result, fail_on_eof); \
    } while (0)

#define fread_int64(file, out, fail_on_eof) \
    do { \
        int result = fread(out, 8, 1, file); \
        handle_fread_result(file, result, fail_on_eof); \
    } while (0)

#define fread_string(arena, file, out, fail_on_eof) \
    do { \
        uint64_t len; \
        int result = fread(&len, 8, 1, file); \
        handle_fread_result(file, result, fail_on_eof); \
        \
        DataString *string = arena_alloc(arena, sizeof(DataString) + len); \
        string->len = len; \
        *out = string; \
        \
        if (len > 0) { \
            int result = fread(string->str, len, 1, file); \
            handle_fread_result(file, result, true); \
        } \
    } while (0)

PottyParser potty_parser_create(Arena *arena, FILE *file) {
    PottyParser parser = {
        .file = file,
        .eof_reached = false,
        .position = 0,
        .arena = arena,
    };

    return parser;
}

PottyParserReadResult potty_parser_read(
    PottyParser *parser,
    SepticTankMutation *out_mutation
) {
    // Read type
    fread_byte(parser->file, (uint8_t *) &out_mutation->type, false);
 
    switch (out_mutation->type) {
        case SEPTIC_TANK_SET: {
            SepticTankSetMutation *set = &out_mutation->set;

            // Expiration
            fread_int64(parser->file, &set->expires_at, true);
            // Key
            fread_string(parser->arena, parser->file, &set->key, true);
            // Value
            fread_string(parser->arena, parser->file, &set->value, true);
        }
        case SEPTIC_TANK_DEL: {
            SepticTankDelMutation *del = &out_mutation->del;
            
            // Key
            fread_string(parser->arena, parser->file, &del->key, true);
        }
        case SEPTIC_TANK_EXPIRE: {
            SepticTankExpireMutation *expire = &out_mutation->expire;
            
            // Key
            fread_string(parser->arena, parser->file, &expire->key, true);
            // Expiration
            fread_int64(parser->file, &expire->expires_at, true);
        }

        case SEPTIC_TANK_GET:
        case SEPTIC_TANK_TTL:
        case SEPTIC_TANK_EXISTS:
        case SEPTIC_TANK_INCRBY:
        default:
            // Unused ops
            UNREACHABLE();
            break;
    }
}
