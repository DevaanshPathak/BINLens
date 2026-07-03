#ifndef BINLENS_HEX_PARSER_H
#define BINLENS_HEX_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "binlens/diagnostic.h"
#include "binlens/firmware_image.h"

/* Basic statistics collected while parsing an Intel HEX file. */
typedef struct BlHexParseStats {
    size_t line_count;
    size_t record_count;
    size_t data_record_count;
    size_t data_byte_count;
    bool saw_eof;
    bool has_start_linear_address;
    uint32_t start_linear_address;
} BlHexParseStats;

/* Initializes HEX parser statistics to default values. */
void bl_hex_parse_stats_init(BlHexParseStats *stats);

/* Parses an Intel HEX file into a firmware image and optional stats object. */
int bl_hex_parse_file(const char *path,
                      BlFirmwareImage *image,
                      BlHexParseStats *stats,
                      BlDiagnostic *diag);

#endif
