#include "binlens/cli.h"
#include "binlens/diagnostic.h"
#include "binlens/format.h"
#include "binlens/firmware_image.h"
#include "binlens/hex_parser.h"
#include "binlens/version.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Infers the input format from the file extension. */
static BlInputFormat infer_format(const char *path)
{
    const char *dot = strrchr(path, '.');

    if (dot == NULL) {
        return BL_FORMAT_AUTO;
    }
    if (strcmp(dot, ".hex") == 0 || strcmp(dot, ".ihex") == 0) {
        return BL_FORMAT_INTEL_HEX;
    }
    if (strcmp(dot, ".bin") == 0) {
        return BL_FORMAT_RAW_BIN;
    }

    return BL_FORMAT_AUTO;
}

/* Infers the input format from the file extension. */
static void print_hex_summary(const BlFirmwareImage *image, const BlHexParseStats *stats)
{
    char start[32];
    char end[32];

    printf("BINLens %s\n\n", BINLENS_VERSION);
    printf("Input\n");
    printf("  File:          %s\n", image->source_path);
    printf("  Format:        %s\n", bl_input_format_name(image->input_format));
    printf("  Records:       %zu\n", stats->record_count);
    printf("  Data records:  %zu\n", stats->data_record_count);
    printf("  Bytes loaded:  %zu\n", image->total_loaded_bytes);
    printf("  Chunks:        %zu\n", image->chunk_count);

    if (image->total_loaded_bytes > 0) {
        bl_format_address(image->address_start, start, sizeof(start));
        bl_format_address(image->address_end, end, sizeof(end));
        printf("  Address span:  %s - %s\n", start, end);
    }

    if (stats->has_start_linear_address) {
        bl_format_address(stats->start_linear_address, start, sizeof(start));
        printf("  Start linear:  %s\n", start);
    }
}

/* Prints a summary for a parsed Intel HEX image. */
int main(int argc, char **argv)
{
    BlCliOptions options;
    BlDiagnostic diag;
    BlFirmwareImage image;
    BlHexParseStats hex_stats;
    BlInputFormat resolved_format;
    const char *program_name = argc > 0 ? argv[0] : "binlens";

    if (bl_cli_parse(argc, argv, &options, &diag) != 0) {
        fprintf(stderr, "%s: %s\n", bl_diag_severity_name(diag.severity), diag.message);
        fprintf(stderr, "Try '%s --help' for usage.\n", program_name);
        return 2;
    }

    if (options.show_help) {
        bl_cli_print_help(stdout, program_name);
        return 0;
    }

    if (options.show_version) {
        printf("BINLens %s\n", BINLENS_VERSION);
        return 0;
    }

    bl_firmware_image_init(&image);
    bl_firmware_image_set_source(&image, options.input_path);
    resolved_format = options.format == BL_FORMAT_AUTO
                          ? infer_format(options.input_path)
                          : options.format;

    if (resolved_format == BL_FORMAT_INTEL_HEX) {
        if (bl_hex_parse_file(options.input_path, &image, &hex_stats, &diag) != 0) {
            fprintf(stderr, "%s: %s\n", bl_diag_severity_name(diag.severity), diag.message);
            bl_firmware_image_free(&image);
            return 1;
        }

        print_hex_summary(&image, &hex_stats);
        bl_firmware_image_free(&image);
        return 0;
    }

    if (resolved_format == BL_FORMAT_RAW_BIN) {
        fprintf(stderr, "error: raw binary loading is not implemented yet\n");
        bl_firmware_image_free(&image);
        return 2;
    }

    fprintf(stderr, "error: could not infer input format; use --format hex or --format bin\n");
    bl_firmware_image_free(&image);
    return 2;
}
