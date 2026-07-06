#include "binlens/diagnostic.h"
#include "binlens/firmware_image.h"
#include "binlens/memmap.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void add_chunk(BlFirmwareImage *image,
                      uint64_t start,
                      const unsigned char *bytes,
                      size_t length,
                      size_t line)
{
    BlDiagnostic diag;

    assert(bl_firmware_image_add_chunk(image,
                                       start,
                                       bytes,
                                       length,
                                       "test",
                                       line,
                                       &diag) == 0);
}

static void test_adjacent_chunks_merge(void)
{
    const unsigned char first[] = {0x01, 0x02};
    const unsigned char second[] = {0x03, 0x04};
    BlFirmwareImage image;
    BlDiagnostic diag;
    const BlMemRegion *region;

    bl_firmware_image_init(&image);
    add_chunk(&image, 0x1000u, first, sizeof(first), 1);
    add_chunk(&image, 0x1002u, second, sizeof(second), 2);

    assert(bl_memmap_reconstruct(&image, &diag) == 0);
    assert(image.region_count == 1);
    assert(image.gap_count == 0);
    assert(image.overlap_count == 0);

    region = bl_firmware_image_region_at(&image, 0);
    assert(region != NULL);
    assert(region->start_address == 0x1000u);
    assert(region->end_address == 0x1003u);
    assert(region->length == 4);
    assert(region->bytes[0] == 0x01);
    assert(region->bytes[3] == 0x04);

    bl_firmware_image_free(&image);
}

static void test_sparse_chunks_create_gap(void)
{
    const unsigned char first[] = {0x01, 0x02};
    const unsigned char second[] = {0x05};
    BlFirmwareImage image;
    BlDiagnostic diag;
    const BlMemGap *gap;

    bl_firmware_image_init(&image);
    add_chunk(&image, 0x1000u, first, sizeof(first), 1);
    add_chunk(&image, 0x1004u, second, sizeof(second), 2);

    assert(bl_memmap_reconstruct(&image, &diag) == 0);
    assert(image.region_count == 2);
    assert(image.gap_count == 1);
    assert(image.overlap_count == 0);

    gap = bl_firmware_image_gap_at(&image, 0);
    assert(gap != NULL);
    assert(gap->start_address == 0x1002u);
    assert(gap->end_address == 0x1003u);
    assert(gap->length == 2);

    bl_firmware_image_free(&image);
}

static void test_identical_overlap_is_reported(void)
{
    const unsigned char first[] = {0x01, 0x02, 0x03};
    const unsigned char second[] = {0x02, 0x03, 0x04};
    BlFirmwareImage image;
    BlDiagnostic diag;
    const BlMemOverlap *overlap;
    const BlMemRegion *region;

    bl_firmware_image_init(&image);
    add_chunk(&image, 0x1000u, first, sizeof(first), 10);
    add_chunk(&image, 0x1001u, second, sizeof(second), 11);

    assert(bl_memmap_reconstruct(&image, &diag) == 0);
    assert(image.region_count == 1);
    assert(image.overlap_count == 1);

    overlap = bl_firmware_image_overlap_at(&image, 0);
    assert(overlap != NULL);
    assert(overlap->start_address == 0x1001u);
    assert(overlap->end_address == 0x1002u);
    assert(overlap->length == 2);
    assert(overlap->kind == BL_OVERLAP_IDENTICAL);

    region = bl_firmware_image_region_at(&image, 0);
    assert(region != NULL);
    assert(region->length == 4);
    assert(region->bytes[0] == 0x01);
    assert(region->bytes[3] == 0x04);

    bl_firmware_image_free(&image);
}

static void test_conflicting_overlap_is_reported(void)
{
    const unsigned char first[] = {0x01, 0x02, 0x03};
    const unsigned char second[] = {0x99, 0x03};
    BlFirmwareImage image;
    BlDiagnostic diag;
    const BlMemOverlap *overlap;
    const BlMemRegion *region;

    bl_firmware_image_init(&image);
    add_chunk(&image, 0x1000u, first, sizeof(first), 20);
    add_chunk(&image, 0x1001u, second, sizeof(second), 21);

    assert(bl_memmap_reconstruct(&image, &diag) == 0);
    assert(image.region_count == 1);
    assert(image.overlap_count == 1);

    overlap = bl_firmware_image_overlap_at(&image, 0);
    assert(overlap != NULL);
    assert(overlap->start_address == 0x1001u);
    assert(overlap->end_address == 0x1002u);
    assert(overlap->kind == BL_OVERLAP_CONFLICTING);
    assert(overlap->first_origin_line == 20);
    assert(overlap->second_origin_line == 21);

    region = bl_firmware_image_region_at(&image, 0);
    assert(region != NULL);
    assert(region->length == 3);
    assert(memcmp(region->bytes, first, sizeof(first)) == 0);

    bl_firmware_image_free(&image);
}

int main(void)
{
    test_adjacent_chunks_merge();
    test_sparse_chunks_create_gap();
    test_identical_overlap_is_reported();
    test_conflicting_overlap_is_reported();
    puts("test_memmap: ok");
    return 0;
}
