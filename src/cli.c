#include "binlens/cli.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Parses an integer string into a uint64_t value. */
static int parse_u64(const char *text, uint64_t *value)
{
    char *end = NULL;
    unsigned long long parsed;

    if (text == NULL || *text == '\0') {
        return -1;
    }

    errno = 0;
    parsed = strtoull(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0') {
        return -1;
    }

    *value = (uint64_t)parsed;
    return 0;
}

/* Parses a positive size value. */
static int parse_size(const char *text, size_t *value)
{
    uint64_t parsed;

    if (parse_u64(text, &parsed) != 0 || parsed == 0 || parsed > (uint64_t)SIZE_MAX) {
        return -1;
    }

    *value = (size_t)parsed;
    return 0;
}


/* Converts a format string into a BlInputFormat value. */
static int parse_format(const char *text, BlInputFormat *format)
{
    if (strcmp(text, "auto") == 0) {
        *format = BL_FORMAT_AUTO;
        return 0;
    }
    if (strcmp(text, "hex") == 0 || strcmp(text, "ihex") == 0) {
        *format = BL_FORMAT_INTEL_HEX;
        return 0;
    }
    if (strcmp(text, "bin") == 0 || strcmp(text, "raw") == 0) {
        *format = BL_FORMAT_RAW_BIN;
        return 0;
    }
    return -1;
}

/* Sets CLI options to their default values. */
void bl_cli_options_init(BlCliOptions *options)
{
    if (options == NULL) {
        return;
    }

    options->input_path[0] = '\0';
    options->format = BL_FORMAT_AUTO;
    options->base_address = 0;
    options->entropy_chunk = BL_DEFAULT_ENTROPY_CHUNK;
    options->show_heatmap = false;
    options->no_color = false;
    options->verbose = false;
    options->show_help = false;
    options->show_version = false;
}

/* Parses command-line arguments into BlCliOptions. */
int bl_cli_parse(int argc, char **argv, BlCliOptions *options, BlDiagnostic *diag)
{
    int i;

    bl_cli_options_init(options);
    bl_diag_clear(diag);

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            options->show_help = true;
        } else if (strcmp(arg, "--version") == 0) {
            options->show_version = true;
        } else if (strcmp(arg, "--heatmap") == 0) {
            options->show_heatmap = true;
        } else if (strcmp(arg, "--no-color") == 0) {
            options->no_color = true;
        } else if (strcmp(arg, "--verbose") == 0 || strcmp(arg, "-v") == 0) {
            options->verbose = true;
        } else if (strcmp(arg, "--format") == 0) {
            if (++i >= argc || parse_format(argv[i], &options->format) != 0) {
                bl_diag_set(diag, BL_DIAG_ERROR, "expected format: auto, hex, or bin");
                return -1;
            }
        } else if (strncmp(arg, "--format=", 9) == 0) {
            if (parse_format(arg + 9, &options->format) != 0) {
                bl_diag_set(diag, BL_DIAG_ERROR, "expected format: auto, hex, or bin");
                return -1;
            }
        } else if (strcmp(arg, "--base") == 0) {
            if (++i >= argc || parse_u64(argv[i], &options->base_address) != 0) {
                bl_diag_set(diag, BL_DIAG_ERROR, "expected numeric base address");
                return -1;
            }
        } else if (strncmp(arg, "--base=", 7) == 0) {
            if (parse_u64(arg + 7, &options->base_address) != 0) {
                bl_diag_set(diag, BL_DIAG_ERROR, "expected numeric base address");
                return -1;
            }
        } else if (strcmp(arg, "--entropy-chunk") == 0) {
            if (++i >= argc || parse_size(argv[i], &options->entropy_chunk) != 0) {
                bl_diag_set(diag, BL_DIAG_ERROR, "expected positive entropy chunk size");
                return -1;
            }
        } else if (strncmp(arg, "--entropy-chunk=", 16) == 0) {
            if (parse_size(arg + 16, &options->entropy_chunk) != 0) {
                bl_diag_set(diag, BL_DIAG_ERROR, "expected positive entropy chunk size");
                return -1;
            }
        } else if (arg[0] == '-') {
            bl_diag_set(diag, BL_DIAG_ERROR, "unknown option: %s", arg);
            return -1;
        } else if (options->input_path[0] != '\0') {
            bl_diag_set(diag, BL_DIAG_ERROR, "only one input file is supported");
            return -1;
        } else {
            /* Store the positional input file path safely. */
            strncpy(options->input_path, arg, sizeof(options->input_path) - 1);
            options->input_path[sizeof(options->input_path) - 1] = '\0';
        }
    }

    if (!options->show_help && !options->show_version && options->input_path[0] == '\0') {
        bl_diag_set(diag, BL_DIAG_ERROR, "missing input file");
        return -1;
    }

    return 0;
}

/* Prints command-line usage information. */
void bl_cli_print_help(FILE *stream, const char *program_name)
{
    fprintf(stream,
            "Usage: %s [options] <firmware.hex|firmware.bin>\n"
            "\n"
            "Options:\n"
            "  --format auto|hex|bin       Input format, default: auto\n"
            "  --base <address>            Base address for raw .bin input\n"
            "  --entropy-chunk <bytes>     Entropy window size, default: 1024\n"
            "  --heatmap                   Show ASCII entropy heatmap\n"
            "  --no-color                  Disable colored output\n"
            "  -v, --verbose               Show detailed diagnostics\n"
            "  -h, --help                  Show this help\n"
            "  --version                   Show version\n",
            program_name);
}
