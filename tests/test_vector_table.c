#include "binlens/diagnostic.h"
#include "binlens/firmware_image.h"
#include "binlens/hex_parser.h"
#include "binlens/memmap.h"
#include "binlens/vector_table.h"

#include <assert.h>
#include <stdio.h>

static void add_chunk(BlFirmwareImage *image,
                      uint64_t start,
                      const unsigned char *bytes,
                      size_t length)
{
    BlDiagnostic diag;

    assert(bl_firmware_image_add_chunk(image, start, bytes, length, "test", 1, &diag) == 0);
}

static void test_high_confidence_vector_table(void)
{
    const unsigned char vector[] = {
        0x00, 0x10, 0x00, 0x20,
        0x01, 0x01, 0x00, 0x08
    };
    const unsigned char code[] = {0x00, 0xBF, 0x00, 0xBF};
    BlFirmwareImage image;
    BlDiagnostic diag;
    BlVectorTableCandidate candidate;

    bl_firmware_image_init(&image);
    add_chunk(&image, 0x08000000u, vector, sizeof(vector));
    add_chunk(&image, 0x08000100u, code, sizeof(code));

    assert(bl_memmap_reconstruct(&image, &diag) == 0);
    assert(bl_vector_table_detect(&image, &candidate) == 0);
    assert(candidate.confidence == BL_VECTOR_CONFIDENCE_HIGH);
    assert(candidate.table_address == 0x08000000u);
    assert(candidate.initial_stack_pointer == 0x20001000u);
    assert(candidate.reset_handler_raw == 0x08000101u);
    assert(candidate.reset_handler_address == 0x08000100u);
    assert(candidate.stack_pointer_valid);
    assert(candidate.stack_pointer_aligned);
    assert(candidate.reset_handler_valid);
    assert(candidate.reset_handler_in_region);

    bl_firmware_image_free(&image);
}

static void test_missing_vector_table(void)
{
    const unsigned char bytes[] = {0, 0, 0, 0, 0, 0, 0, 0};
    BlFirmwareImage image;
    BlDiagnostic diag;
    BlVectorTableCandidate candidate;

    bl_firmware_image_init(&image);
    add_chunk(&image, 0x08000000u, bytes, sizeof(bytes));

    assert(bl_memmap_reconstruct(&image, &diag) == 0);
    assert(bl_vector_table_detect(&image, &candidate) == 0);
    assert(candidate.confidence == BL_VECTOR_CONFIDENCE_NONE);

    bl_firmware_image_free(&image);
}

static void test_stm32_like_hex_fixture(void)
{
    BlFirmwareImage image;
    BlHexParseStats stats;
    BlDiagnostic diag;
    BlVectorTableCandidate candidate;

    bl_firmware_image_init(&image);
    assert(bl_hex_parse_file("tests/fixtures/stm32_vector.ihex", &image, &stats, &diag) == 0);
    assert(bl_memmap_reconstruct(&image, &diag) == 0);
    assert(bl_vector_table_detect(&image, &candidate) == 0);

    assert(stats.has_start_linear_address);
    assert(stats.start_linear_address == 0x08000101u);
    assert(image.region_count == 2);
    assert(image.gap_count == 1);
    assert(candidate.confidence == BL_VECTOR_CONFIDENCE_HIGH);

    bl_firmware_image_free(&image);
}

int main(void)
{
    test_high_confidence_vector_table();
    test_missing_vector_table();
    test_stm32_like_hex_fixture();
    puts("test_vector_table: ok");
    return 0;
}
