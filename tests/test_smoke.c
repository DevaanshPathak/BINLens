#include "binlens/cli.h"
#include "binlens/diagnostic.h"
#include "binlens/entropy.h"
#include "binlens/firmware_image.h"

#include <assert.h>
#include <stdio.h>

/* Verifies that the help flag parses successfully. */
static void test_cli_help(void)
{
    char *argv[] = {"binlens", "--help"};
    BlCliOptions options;
    BlDiagnostic diag;

    assert(bl_cli_parse(2, argv, &options, &diag) == 0);
    assert(options.show_help);
    assert(options.entropy_chunk == BL_DEFAULT_ENTROPY_CHUNK);
}

/* Verifies CLI parsing for raw binary format and base address. */
static void test_cli_base_and_format(void)
{
    char *argv[] = {"binlens", "--format", "bin", "--base", "0x08000000", "app.bin"};
    BlCliOptions options;
    BlDiagnostic diag;

    assert(bl_cli_parse(6, argv, &options, &diag) == 0);
    assert(options.format == BL_FORMAT_RAW_BIN);
    assert(options.base_address == 0x08000000u);
}

/* Verifies basic entropy and heatmap behaviour. */
static void test_entropy_heatmap_symbols(void)
{
    assert(bl_entropy_shannon(0, 0) == 0.0);
    assert(bl_entropy_heatmap_symbol(0.0) == '.');
    assert(bl_entropy_heatmap_symbol(7.9) == '%');
}

/* Verifies firmware image initialization and source path storage. */
static void test_firmware_image_init(void)
{
    BlFirmwareImage image;

    bl_firmware_image_init(&image);
    assert(image.input_format == BL_FORMAT_AUTO);
    assert(image.chunk_count == 0);

    bl_firmware_image_set_source(&image, "firmware.hex");
    assert(image.source_path[0] == 'f');
    bl_firmware_image_free(&image);
}

int main(void)
{
    test_cli_help();
    test_cli_base_and_format();
    test_entropy_heatmap_symbols();
    test_firmware_image_init();
    puts("test_smoke: ok");
    return 0;
}
