#include "binlens/bin_loader.h"
#include "binlens/cli.h"
#include "binlens/diagnostic.h"
#include "binlens/entropy.h"
#include "binlens/format.h"
#include "binlens/firmware_image.h"
#include "binlens/hex_parser.h"
#include "binlens/memmap.h"
#include "binlens/vector_table.h"
#include "binlens/version.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int ascii_lower(int ch)
{
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A' + 'a';
    }
    return ch;
}

static int extension_equals(const char *left, const char *right)
{
    if (left == NULL || right == NULL) {
        return 0;
    }

    while (*left != '\0' && *right != '\0') {
        if (ascii_lower((unsigned char)*left) != ascii_lower((unsigned char)*right)) {
            return 0;
        }
        left++;
        right++;
    }

    return *left == '\0' && *right == '\0';
}

static BlInputFormat infer_format(const char *path)
{
    const char *dot = strrchr(path, '.');

    if (dot == NULL) {
        return BL_FORMAT_AUTO;
    }
    if (extension_equals(dot, ".hex") || extension_equals(dot, ".ihex")) {
        return BL_FORMAT_INTEL_HEX;
    }
    if (extension_equals(dot, ".bin")) {
        return BL_FORMAT_RAW_BIN;
    }

    return BL_FORMAT_AUTO;
}

static void print_address_span(const BlFirmwareImage *image)
{
    char start[32];
    char end[32];

    if (image->total_loaded_bytes == 0) {
        printf("  Address span:  none\n");
        return;
    }

    bl_format_address(image->address_start, start, sizeof(start));
    bl_format_address(image->address_end, end, sizeof(end));
    printf("  Address span:  %s - %s\n", start, end);
}

static void print_input_summary(const BlFirmwareImage *image, const BlHexParseStats *hex_stats)
{
    char size[32];
    char start_linear[32];

    bl_format_size(image->total_loaded_bytes, size, sizeof(size));

    printf("Input\n");
    printf("  File:          %s\n", image->source_path);
    printf("  Format:        %s\n", bl_input_format_name(image->input_format));
    if (hex_stats != NULL) {
        printf("  Records:       %zu\n", hex_stats->record_count);
        printf("  Data records:  %zu\n", hex_stats->data_record_count);
    }
    printf("  Bytes loaded:  %zu (%s)\n", image->total_loaded_bytes, size);
    printf("  Source chunks: %zu\n", image->chunk_count);
    print_address_span(image);

    if (hex_stats != NULL && hex_stats->has_start_linear_address) {
        bl_format_address(hex_stats->start_linear_address, start_linear, sizeof(start_linear));
        printf("  Start linear:  %s\n", start_linear);
    }
}

static void print_source_chunks(const BlFirmwareImage *image)
{
    size_t i;

    if (image->chunk_count == 0) {
        return;
    }

    printf("\nSource Chunks\n");
    printf("  #  Start       End         Size\n");
    for (i = 0; i < image->chunk_count; i++) {
        const BlSourceChunk *chunk = &image->chunks[i];
        char start[32];
        char end[32];
        char size[32];

        bl_format_address(chunk->start_address, start, sizeof(start));
        bl_format_address(chunk->end_address, end, sizeof(end));
        bl_format_size(chunk->length, size, sizeof(size));
        printf("  %-2zu %-10s  %-10s  %s\n", i, start, end, size);
    }
}

static size_t min_size(size_t left, size_t right)
{
    return left < right ? left : right;
}

static void print_vector_table(const BlVectorTableCandidate *candidate)
{
    char table[32];
    char sp[32];
    char reset_raw[32];
    char reset_address[32];

    printf("\nARM Cortex-M Vector Table\n");
    if (candidate->confidence == BL_VECTOR_CONFIDENCE_NONE) {
        printf("  Candidate:     not found\n");
        return;
    }

    bl_format_address(candidate->table_address, table, sizeof(table));
    bl_format_address(candidate->initial_stack_pointer, sp, sizeof(sp));
    bl_format_address(candidate->reset_handler_raw, reset_raw, sizeof(reset_raw));
    bl_format_address(candidate->reset_handler_address, reset_address, sizeof(reset_address));

    printf("  Candidate:     %s\n", table);
    printf("  Initial SP:    %s  %s, %s\n",
           sp,
           candidate->stack_pointer_valid ? "valid SRAM range" : "outside common SRAM range",
           candidate->stack_pointer_aligned ? "aligned" : "unaligned");
    printf("  Reset handler: %s  target %s, %s, %s\n",
           reset_raw,
           reset_address,
           candidate->reset_handler_valid ? "Thumb bit set" : "Thumb bit clear",
           candidate->reset_handler_in_region ? "populated target" : "target not loaded");
    printf("  Confidence:    %s (score %u)\n",
           bl_vector_confidence_name(candidate->confidence),
           candidate->score);
}

static void print_regions(const BlFirmwareImage *image)
{
    size_t i;

    printf("\nMemory Regions\n");
    if (image->region_count == 0) {
        printf("  none\n");
        return;
    }

    printf("  #  Start       End         Size     Entropy\n");
    for (i = 0; i < image->region_count; i++) {
        const BlMemRegion *region = &image->regions[i];
        char start[32];
        char end[32];
        char size[32];

        bl_format_address(region->start_address, start, sizeof(start));
        bl_format_address(region->end_address, end, sizeof(end));
        bl_format_size(region->length, size, sizeof(size));
        printf("  %-2zu %-10s  %-10s  %-8s %.2f\n", i, start, end, size, region->entropy);
    }
}

static void print_gaps(const BlFirmwareImage *image)
{
    size_t i;

    printf("\nGaps\n");
    if (image->gap_count == 0) {
        printf("  none\n");
        return;
    }

    printf("  #  Start       End         Size\n");
    for (i = 0; i < image->gap_count; i++) {
        const BlMemGap *gap = &image->gaps[i];
        char start[32];
        char end[32];
        char size[32];

        bl_format_address(gap->start_address, start, sizeof(start));
        bl_format_address(gap->end_address, end, sizeof(end));
        bl_format_u64_size(gap->length, size, sizeof(size));
        printf("  %-2zu %-10s  %-10s  %s\n", i, start, end, size);
    }
}

static void print_overlaps(const BlFirmwareImage *image)
{
    size_t i;

    printf("\nOverlaps\n");
    if (image->overlap_count == 0) {
        printf("  none\n");
        return;
    }

    printf("  #  Start       End         Size    Kind         Origins\n");
    for (i = 0; i < image->overlap_count; i++) {
        const BlMemOverlap *overlap = &image->overlaps[i];
        char start[32];
        char end[32];
        char size[32];

        bl_format_address(overlap->start_address, start, sizeof(start));
        bl_format_address(overlap->end_address, end, sizeof(end));
        bl_format_size(overlap->length, size, sizeof(size));

        if (overlap->first_origin_line != 0 || overlap->second_origin_line != 0) {
            printf("  %-2zu %-10s  %-10s  %-7s %-11s line %zu / line %zu\n",
                   i,
                   start,
                   end,
                   size,
                   bl_overlap_kind_name(overlap->kind),
                   overlap->first_origin_line,
                   overlap->second_origin_line);
        } else {
            printf("  %-2zu %-10s  %-10s  %-7s %-11s chunk %zu / chunk %zu\n",
                   i,
                   start,
                   end,
                   size,
                   bl_overlap_kind_name(overlap->kind),
                   overlap->first_chunk_index,
                   overlap->second_chunk_index);
        }
    }
}

static void print_entropy_chunks(const BlFirmwareImage *image, size_t chunk_size)
{
    size_t i;

    if (chunk_size == 0 || image->region_count == 0) {
        return;
    }

    printf("\nEntropy Chunks, chunk=%zu bytes\n", chunk_size);
    printf("  Start       End         Size     Entropy  Symbol\n");

    for (i = 0; i < image->region_count; i++) {
        const BlMemRegion *region = &image->regions[i];
        size_t offset;

        for (offset = 0; offset < region->length; offset += chunk_size) {
            size_t length = min_size(chunk_size, region->length - offset);
            uint64_t start_address = region->start_address + (uint64_t)offset;
            uint64_t end_address = start_address + (uint64_t)length - 1u;
            double entropy = bl_entropy_shannon(region->bytes + offset, length);
            char start[32];
            char end[32];
            char size[32];

            bl_format_address(start_address, start, sizeof(start));
            bl_format_address(end_address, end, sizeof(end));
            bl_format_size(length, size, sizeof(size));
            printf("  %-10s  %-10s  %-8s %.2f     %c\n",
                   start,
                   end,
                   size,
                   entropy,
                   bl_entropy_heatmap_symbol(entropy));
        }
    }
}

static void print_entropy_heatmap(const BlFirmwareImage *image, size_t chunk_size)
{
    size_t i;

    if (chunk_size == 0 || image->region_count == 0) {
        return;
    }

    printf("\nEntropy Heatmap, chunk=%zu bytes\n", chunk_size);
    for (i = 0; i < image->region_count; i++) {
        const BlMemRegion *region = &image->regions[i];
        size_t offset = 0;

        while (offset < region->length) {
            size_t line_start = offset;
            size_t symbols = 0;
            char address[32];

            bl_format_address(region->start_address + (uint64_t)line_start,
                              address,
                              sizeof(address));
            printf("  %s  ", address);

            while (offset < region->length && symbols < 32u) {
                size_t length = min_size(chunk_size, region->length - offset);
                double entropy = bl_entropy_shannon(region->bytes + offset, length);

                putchar(bl_entropy_heatmap_symbol(entropy));
                offset += length;
                symbols++;
            }
            putchar('\n');
        }
    }

    printf("\nLegend\n");
    printf("  . 0.0-1.0   : 1.0-2.5   - 2.5-4.0   = 4.0-5.5\n");
    printf("  + 5.5-6.5   * 6.5-7.2   # 7.2-7.8   %% 7.8-8.0\n");
}

static void print_summary(const BlFirmwareImage *image,
                          const BlHexParseStats *hex_stats,
                          const BlCliOptions *options)
{
    BlVectorTableCandidate vector_candidate;

    bl_vector_table_detect(image, &vector_candidate);

    printf("BINLens %s\n\n", BINLENS_VERSION);
    print_input_summary(image, hex_stats);
    print_vector_table(&vector_candidate);
    if (options->verbose) {
        print_source_chunks(image);
    }
    print_regions(image);
    print_gaps(image);
    print_overlaps(image);
    if (options->verbose) {
        print_entropy_chunks(image, options->entropy_chunk);
    }
    if (options->show_heatmap) {
        print_entropy_heatmap(image, options->entropy_chunk);
    }
}

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
        if (bl_memmap_reconstruct(&image, &diag) != 0) {
            fprintf(stderr, "%s: %s\n", bl_diag_severity_name(diag.severity), diag.message);
            bl_firmware_image_free(&image);
            return 1;
        }

        print_summary(&image, &hex_stats, &options);
        bl_firmware_image_free(&image);
        return 0;
    }

    if (resolved_format == BL_FORMAT_RAW_BIN) {
        if (bl_bin_load_file(options.input_path, options.base_address, &image, &diag) != 0) {
            fprintf(stderr, "%s: %s\n", bl_diag_severity_name(diag.severity), diag.message);
            bl_firmware_image_free(&image);
            return 1;
        }
        if (bl_memmap_reconstruct(&image, &diag) != 0) {
            fprintf(stderr, "%s: %s\n", bl_diag_severity_name(diag.severity), diag.message);
            bl_firmware_image_free(&image);
            return 1;
        }

        print_summary(&image, NULL, &options);
        bl_firmware_image_free(&image);
        return 0;
    }

    fprintf(stderr,
            "error: could not infer input format for '%s'; use --format hex or --format bin\n",
            options.input_path);
    bl_firmware_image_free(&image);
    return 2;
}
