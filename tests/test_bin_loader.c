#include "binlens/bin_loader.h"
#include "binlens/diagnostic.h"
#include "binlens/firmware_image.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void write_fixture(const char *path, const unsigned char *bytes, size_t length)
{
    FILE *file = fopen(path, "wb");

    assert(file != NULL);
    assert(fwrite(bytes, 1, length, file) == length);
    assert(fclose(file) == 0);
}

static void test_bin_load_with_base_address(void)
{
    const char *path = "tests/fixtures/raw_loader.bin";
    const unsigned char bytes[] = {0x11, 0x22, 0x33, 0x44};
    BlFirmwareImage image;
    BlDiagnostic diag;
    const BlSourceChunk *chunk;

    write_fixture(path, bytes, sizeof(bytes));
    bl_firmware_image_init(&image);

    assert(bl_bin_load_file(path, 0x08000000u, &image, &diag) == 0);
    assert(image.input_format == BL_FORMAT_RAW_BIN);
    assert(image.chunk_count == 1);
    assert(image.total_loaded_bytes == 4);
    assert(image.address_start == 0x08000000u);
    assert(image.address_end == 0x08000003u);

    chunk = bl_firmware_image_chunk_at(&image, 0);
    assert(chunk != NULL);
    assert(chunk->start_address == 0x08000000u);
    assert(chunk->end_address == 0x08000003u);
    assert(chunk->length == 4);
    assert(memcmp(chunk->bytes, bytes, sizeof(bytes)) == 0);

    bl_firmware_image_free(&image);
    remove(path);
}

static void test_bin_load_rejects_address_overflow(void)
{
    const char *path = "tests/fixtures/raw_overflow.bin";
    const unsigned char bytes[] = {0xAA, 0xBB, 0xCC, 0xDD};
    BlFirmwareImage image;
    BlDiagnostic diag;

    write_fixture(path, bytes, sizeof(bytes));
    bl_firmware_image_init(&image);

    assert(bl_bin_load_file(path, UINT64_MAX - 1u, &image, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
    assert(strstr(diag.message, "overflows") != NULL);
    assert(image.chunk_count == 0);

    bl_firmware_image_free(&image);
    remove(path);
}

int main(void)
{
    test_bin_load_with_base_address();
    test_bin_load_rejects_address_overflow();
    puts("test_bin_loader: ok");
    return 0;
}
