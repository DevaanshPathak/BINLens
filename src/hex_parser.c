#include "binlens/hex_parser.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Maximum supported Intel HEX line length. */
#define BL_HEX_MAX_LINE 1024u

/* Converts one hex character into a 4-bit value. */
static int hex_nibble(int ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return -1;
}

/* Parses two hex characters from a line into one byte. */
static int parse_hex_byte(const char *line, size_t offset, unsigned char *value)
{
    int high = hex_nibble((unsigned char)line[offset]);
    int low = hex_nibble((unsigned char)line[offset + 1u]);

    if (high < 0 || low < 0) {
        return -1;
    }

    *value = (unsigned char)((high << 4) | low);
    return 0;
}

/* Remove trailing newline characters from a line. */
static void strip_line_ending(char *line)
{
    size_t length = strlen(line);

    while (length > 0) {
        char last = line[length - 1u];
        if (last != '\n' && last != '\r') {
            break;
        }
        line[--length] = '\0';
    }
}

/* Initializes Intel HEX parser statistics. */
void bl_hex_parse_stats_init(BlHexParseStats *stats)
{
    if (stats == 0) {
        return;
    }

    stats->line_count = 0;
    stats->record_count = 0;
    stats->data_record_count = 0;
    stats->data_byte_count = 0;
    stats->saw_eof = false;
    stats->has_start_linear_address = false;
    stats->start_linear_address = 0;
}

/* Parses an Intel HEX file into a firmware image. */
int bl_hex_parse_file(const char *path,
                      BlFirmwareImage *image,
                      BlHexParseStats *stats,
                      BlDiagnostic *diag)
{
    FILE *file;
    char line[BL_HEX_MAX_LINE];
    uint64_t address_base = 0;

    bl_hex_parse_stats_init(stats);
    bl_diag_clear(diag);

    if (path == NULL || image == NULL || stats == NULL) {
        bl_diag_set(diag, BL_DIAG_ERROR, "invalid Intel HEX parser argument");
        return -1;
    }

    file = fopen(path, "r");
    if (file == NULL) {
        char cwd[256];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            bl_diag_set(diag,
                        BL_DIAG_ERROR,
                        "could not open Intel HEX file: %s (%s); current directory: %s",
                        path,
                        strerror(errno),
                        cwd);
        } else {
            bl_diag_set(diag,
                        BL_DIAG_ERROR,
                        "could not open Intel HEX file: %s (%s)",
                        path,
                        strerror(errno));
        }
        return -1;
    }

    image->input_format = BL_FORMAT_INTEL_HEX;
    bl_firmware_image_set_source(image, path);

    while (fgets(line, sizeof(line), file) != NULL) {
        size_t length;
        unsigned char byte_count;
        unsigned char address_hi;
        unsigned char address_lo;
        unsigned char record_type;
        unsigned char checksum;
        unsigned char data[255];
        unsigned int sum;
        unsigned char computed_checksum;
        uint16_t offset_address;
        size_t i;
        size_t expected_length;

        stats->line_count++;

        length = strlen(line);
        if (length == sizeof(line) - 1u && line[length - 1u] != '\n') {
            bl_diag_set(diag,
                        BL_DIAG_ERROR,
                        "Intel HEX line %zu exceeds maximum supported length",
                        stats->line_count);
            goto fail;
        }

        strip_line_ending(line);
        length = strlen(line);
        if (length == 0) {
            continue;
        }

        if (stats->saw_eof) {
            bl_diag_set(diag,
                        BL_DIAG_ERROR,
                        "Intel HEX record found after EOF on line %zu",
                        stats->line_count);
            goto fail;
        }

        if (line[0] != ':') {
            bl_diag_set(diag,
                        BL_DIAG_ERROR,
                        "Intel HEX record on line %zu does not start with ':'",
                        stats->line_count);
            goto fail;
        }

        if (length < 11u) {
            bl_diag_set(diag,
                        BL_DIAG_ERROR,
                        "Intel HEX record on line %zu is too short",
                        stats->line_count);
            goto fail;
        }

        if (parse_hex_byte(line, 1u, &byte_count) != 0 ||
            parse_hex_byte(line, 3u, &address_hi) != 0 ||
            parse_hex_byte(line, 5u, &address_lo) != 0 ||
            parse_hex_byte(line, 7u, &record_type) != 0) {
            bl_diag_set(diag,
                        BL_DIAG_ERROR,
                        "Intel HEX record on line %zu contains invalid header hex",
                        stats->line_count);
            goto fail;
        }

        expected_length = 11u + ((size_t)byte_count * 2u);
        if (length != expected_length) {
            bl_diag_set(diag,
                        BL_DIAG_ERROR,
                        "Intel HEX record on line %zu has length %zu, expected %zu",
                        stats->line_count,
                        length,
                        expected_length);
            goto fail;
        }

        sum = byte_count + address_hi + address_lo + record_type;
        for (i = 0; i < byte_count; i++) {
            if (parse_hex_byte(line, 9u + (i * 2u), &data[i]) != 0) {
                bl_diag_set(diag,
                            BL_DIAG_ERROR,
                            "Intel HEX record on line %zu contains invalid data hex",
                            stats->line_count);
                goto fail;
            }
            sum += data[i];
        }

        if (parse_hex_byte(line, 9u + ((size_t)byte_count * 2u), &checksum) != 0) {
            bl_diag_set(diag,
                        BL_DIAG_ERROR,
                        "Intel HEX record on line %zu contains invalid checksum hex",
                        stats->line_count);
            goto fail;
        }

        computed_checksum = (unsigned char)(0u - (sum & 0xFFu));
        if (checksum != computed_checksum) {
            bl_diag_set(diag,
                        BL_DIAG_ERROR,
                        "Intel HEX checksum mismatch on line %zu: record has 0x%02X, computed 0x%02X",
                        stats->line_count,
                        checksum,
                        computed_checksum);
            goto fail;
        }

        stats->record_count++;
        offset_address = (uint16_t)(((uint16_t)address_hi << 8) | address_lo);

        switch (record_type) {
        case 0x00:
            if (bl_firmware_image_add_chunk(image,
                                            address_base + offset_address,
                                            data,
                                            byte_count,
                                            path,
                                            stats->line_count,
                                            diag) != 0) {
                goto fail;
            }
            stats->data_record_count++;
            stats->data_byte_count += byte_count;
            break;

        case 0x01:
            if (byte_count != 0 || offset_address != 0) {
                bl_diag_set(diag,
                            BL_DIAG_ERROR,
                            "Intel HEX EOF record on line %zu must have zero length and address",
                            stats->line_count);
                goto fail;
            }
            stats->saw_eof = true;
            break;

        case 0x02:
            if (byte_count != 2 || offset_address != 0) {
                bl_diag_set(diag,
                            BL_DIAG_ERROR,
                            "Intel HEX extended segment address record on line %zu must contain two bytes at address 0",
                            stats->line_count);
                goto fail;
            }
            address_base = (uint64_t)(((uint16_t)data[0] << 8) | data[1]) << 4;
            break;

        case 0x04:
            if (byte_count != 2 || offset_address != 0) {
                bl_diag_set(diag,
                            BL_DIAG_ERROR,
                            "Intel HEX extended linear address record on line %zu must contain two bytes at address 0",
                            stats->line_count);
                goto fail;
            }
            address_base = (uint64_t)(((uint16_t)data[0] << 8) | data[1]) << 16;
            break;

        case 0x05:
            if (byte_count != 4 || offset_address != 0) {
                bl_diag_set(diag,
                            BL_DIAG_ERROR,
                            "Intel HEX start linear address record on line %zu must contain four bytes at address 0",
                            stats->line_count);
                goto fail;
            }
            stats->has_start_linear_address = true;
            stats->start_linear_address = ((uint32_t)data[0] << 24) |
                                          ((uint32_t)data[1] << 16) |
                                          ((uint32_t)data[2] << 8) |
                                          (uint32_t)data[3];
            break;

        default:
            bl_diag_set(diag,
                        BL_DIAG_ERROR,
                        "unsupported Intel HEX record type 0x%02X on line %zu",
                        record_type,
                        stats->line_count);
            goto fail;
        }
    }

    if (ferror(file)) {
        bl_diag_set(diag, BL_DIAG_ERROR, "error while reading Intel HEX file: %s", path);
        goto fail;
    }

    if (!stats->saw_eof) {
        bl_diag_set(diag, BL_DIAG_ERROR, "Intel HEX file is missing EOF record");
        goto fail;
    }

    fclose(file);
    return 0;

fail:
    fclose(file);
    bl_firmware_image_free(image);
    return -1;
}
