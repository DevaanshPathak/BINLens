#ifndef BINLENS_CLI_H
#define BINLENS_CLI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "binlens/diagnostic.h"
#include "binlens/firmware_image.h"

/* Default number of bytes processed per chunk during entropy analysis. */
#define BL_DEFAULT_ENTROPY_CHUNK 1024u

/* Maximum size reserved for storing input file paths. */
#define BL_PATH_BUFFER_SIZE 512u

/**
 * Stores command-line options parsed from the user input.
 *
 * This structure represents all CLI-level configuration needed by BinLens,
 * including the input file path, selected input format, base load address,
 * entropy settings, output display flags, and informational flags such as
 * help and version.
 */
typedef struct BlCliOptions {
    /* Path to the firmware or binary input file. */
    char input_path[BL_PATH_BUFFER_SIZE];

    /* Input file format selected by the user or detected by the CLI. */
    BlInputFormat format;

    /* Base address used when loading raw binary files. */
    uint64_t base_address;

    /* Chunk size used for entropy calculations. */
    size_t entropy_chunk;

    /* Whether to display an entropy heatmap in the output. */
    bool show_heatmap;

    /* Whether colored terminal output should be disabled. */
    bool no_color;

    /* Whether verbose diagnostic or processing output should be shown. */
    bool verbose;

    /* Whether the CLI should print usage/help information. */
    bool show_help;

    /* Whether the CLI should print version information. */
    bool show_version;
} BlCliOptions;

/**
 * Initializes CLI options with default values.
 *
 * This should be called before parsing command-line arguments so that all
 * fields have predictable defaults.
 *
 * @param options CLI options structure to initialize.
 */
void bl_cli_options_init(BlCliOptions *options);

/**
 * Parses command-line arguments into a CLI options structure.
 *
 * The parser reads user-provided arguments, validates supported flags and
 * values, and stores the resulting configuration in the provided options
 * structure. Any parsing errors are reported through the diagnostic object
 * when provided.
 *
 * @param argc    Number of command-line arguments.
 * @param argv    Command-line argument array.
 * @param options CLI options structure that receives parsed values.
 * @param diag    Diagnostic object used for reporting errors or warnings.
 *
 * @return 0 on success, or a non-zero value on failure.
 */
int bl_cli_parse(int argc, char **argv, BlCliOptions *options, BlDiagnostic *diag);

/**
 * Prints CLI usage and help information.
 *
 * The help text is written to the provided output stream, allowing callers
 * to print either to stdout or stderr depending on context.
 *
 * @param stream       Output stream where help text should be written.
 * @param program_name Name of the executable, usually argv[0].
 */
void bl_cli_print_help(FILE *stream, const char *program_name);

#endif