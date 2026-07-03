#include "binlens/diagnostic.h"
#include "binlens/firmware_image.h"
#include "binlens/hex_parser.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Verifies parsing of a basic Intel HEX file. */
static void test_minimal_hex(void)
{
    BlFirmwareImage image;
    BlHexParseStats stats;
    BlDiagnostic diag;
    const BlSourceChunk *chunk;

    bl_firmware_image_init(&image);
    assert(bl_hex_parse_file("tests/fixtures/minimal.ihex", &image, &stats, &diag) == 0);

    assert(stats.line_count == 2);
    assert(stats.record_count == 2);
    assert(stats.data_record_count == 1);
    assert(stats.data_byte_count == 16);
    assert(stats.saw_eof);
    assert(!stats.has_start_linear_address);

    assert(image.input_format == BL_FORMAT_INTEL_HEX);
    assert(image.chunk_count == 1);
    assert(image.total_loaded_bytes == 16);
    assert(image.address_start == 0);
    assert(image.address_end == 15);

    chunk = bl_firmware_image_chunk_at(&image, 0);
    assert(chunk != NULL);
    assert(chunk->start_address == 0);
    assert(chunk->end_address == 15);
    assert(chunk->length == 16);
    assert(chunk->bytes[0] == 0x01);
    assert(chunk->bytes[15] == 0x10);
    assert(chunk->origin_line == 1);

    bl_firmware_image_free(&image);
}

/* Verifies extended linear addressing and start address records. */
static void test_extended_linear_and_start_address(void)
{
    BlFirmwareImage image;
    BlHexParseStats stats;
    BlDiagnostic diag;
    const BlSourceChunk *chunk;

    bl_firmware_image_init(&image);
    assert(bl_hex_parse_file("tests/fixtures/extended_linear.ihex", &image, &stats, &diag) == 0);

    assert(stats.record_count == 4);
    assert(stats.data_record_count == 1);
    assert(stats.has_start_linear_address);
    assert(stats.start_linear_address == 0x08001234u);

    chunk = bl_firmware_image_chunk_at(&image, 0);
    assert(chunk != NULL);
    assert(chunk->start_address == 0x08000000u);
    assert(chunk->end_address == 0x08000003u);
    assert(chunk->length == 4);
    assert(chunk->bytes[0] == 0x01);
    assert(chunk->bytes[3] == 0x04);

    bl_firmware_image_free(&image);
}

/* Verifies extended segment address handling.*/
static void test_extended_segment_address(void)
{
    BlFirmwareImage image;
    BlHexParseStats stats;
    BlDiagnostic diag;
    const BlSourceChunk *chunk;

    bl_firmware_image_init(&image);
    assert(bl_hex_parse_file("tests/fixtures/extended_segment.ihex", &image, &stats, &diag) == 0);

    chunk = bl_firmware_image_chunk_at(&image, 0);
    assert(chunk != NULL);
    assert(chunk->start_address == 0x00012350u);
    assert(chunk->end_address == 0x00012350u);
    assert(chunk->length == 1);
    assert(chunk->bytes[0] == 0xAA);

    bl_firmware_image_free(&image);
}

/* Verifies checksum errors include useful diagnostics. */
static void test_checksum_failure_reports_line(void)
{
    BlFirmwareImage image;
    BlHexParseStats stats;
    BlDiagnostic diag;

    bl_firmware_image_init(&image);
    assert(bl_hex_parse_file("tests/fixtures/bad_checksum.ihex", &image, &stats, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
    assert(strstr(diag.message, "line 1") != NULL);
    assert(strstr(diag.message, "checksum") != NULL);
    assert(image.chunk_count == 0);

    bl_firmware_image_free(&image);
}

/* Verifies records after EOF are rejected. */
static void test_record_after_eof_fails(void)
{
    BlFirmwareImage image;
    BlHexParseStats stats;
    BlDiagnostic diag;

    bl_firmware_image_init(&image);
    assert(bl_hex_parse_file("tests/fixtures/after_eof.ihex", &image, &stats, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
    assert(strstr(diag.message, "after EOF") != NULL);

    bl_firmware_image_free(&image);
}

int main(void)
{
    test_minimal_hex();
    test_extended_linear_and_start_address();
    test_extended_segment_address();
    test_checksum_failure_reports_line();
    test_record_after_eof_fails();
    puts("test_hex_parser: ok");
    return 0;
}
