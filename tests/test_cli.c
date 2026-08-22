#include "binlens/cli.h"
#include "binlens/diagnostic.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_cli_options_init_defaults(void)
{
    BlCliOptions options;
    bl_cli_options_init(&options);
    assert(options.format == BL_FORMAT_AUTO);
    assert(options.base_address == 0);
    assert(options.entropy_chunk == BL_DEFAULT_ENTROPY_CHUNK);
    assert(!options.show_heatmap);
    assert(!options.no_color);
    assert(!options.verbose);
    assert(!options.show_help);
    assert(!options.show_version);
    assert(options.input_path[0] == '\0');
}

static void test_cli_parse_help(void)
{
    char *argv[] = {"binlens", "--help"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(2, argv, &options, &diag) == 0);
    assert(options.show_help);
}

static void test_cli_parse_version(void)
{
    char *argv[] = {"binlens", "--version"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(2, argv, &options, &diag) == 0);
    assert(options.show_version);
}

static void test_cli_parse_verbose(void)
{
    char *argv[] = {"binlens", "-v", "firmware.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(3, argv, &options, &diag) == 0);
    assert(options.verbose);
    assert(strcmp(options.input_path, "firmware.hex") == 0);
}

static void test_cli_parse_heatmap(void)
{
    char *argv[] = {"binlens", "--heatmap", "firmware.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(3, argv, &options, &diag) == 0);
    assert(options.show_heatmap);
}

static void test_cli_parse_base_and_format(void)
{
    char *argv[] = {"binlens", "--format", "bin", "--base", "0x08000000", "app.bin"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(6, argv, &options, &diag) == 0);
    assert(options.format == BL_FORMAT_RAW_BIN);
    assert(options.base_address == 0x08000000u);
    assert(strcmp(options.input_path, "app.bin") == 0);
}

static void test_cli_parse_format_equals(void)
{
    char *argv[] = {"binlens", "--format=hex", "fw.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(3, argv, &options, &diag) == 0);
    assert(options.format == BL_FORMAT_INTEL_HEX);
}

static void test_cli_parse_base_equals(void)
{
    char *argv[] = {"binlens", "--base=0x10000000", "fw.bin"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(3, argv, &options, &diag) == 0);
    assert(options.base_address == 0x10000000u);
}

static void test_cli_parse_entropy_chunk_equals(void)
{
    char *argv[] = {"binlens", "--entropy-chunk=512", "fw.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(3, argv, &options, &diag) == 0);
    assert(options.entropy_chunk == 512);
}

static void test_cli_parse_entropy_chunk_flag(void)
{
    char *argv[] = {"binlens", "--entropy-chunk", "256", "fw.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(4, argv, &options, &diag) == 0);
    assert(options.entropy_chunk == 256);
}

static void test_cli_parse_no_color(void)
{
    char *argv[] = {"binlens", "--no-color", "fw.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(3, argv, &options, &diag) == 0);
    assert(options.no_color);
}

static void test_cli_missing_input_file(void)
{
    char *argv[] = {"binlens"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(1, argv, &options, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
    assert(strstr(diag.message, "missing input file") != NULL);
}

static void test_cli_unknown_option(void)
{
    char *argv[] = {"binlens", "--bogus", "fw.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(3, argv, &options, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
    assert(strstr(diag.message, "unknown option") != NULL);
}

static void test_cli_bad_base_not_a_number(void)
{
    char *argv[] = {"binlens", "--base", "notanumber", "fw.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(4, argv, &options, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
    assert(strstr(diag.message, "expected numeric base address") != NULL);
}

static void test_cli_bad_base_equals_not_a_number(void)
{
    char *argv[] = {"binlens", "--base=notanumber", "fw.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(3, argv, &options, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
    assert(strstr(diag.message, "expected numeric base address") != NULL);
}

static void test_cli_bad_entropy_chunk_zero(void)
{
    char *argv[] = {"binlens", "--entropy-chunk", "0", "fw.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(4, argv, &options, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
}

static void test_cli_bad_entropy_chunk_not_a_number(void)
{
    char *argv[] = {"binlens", "--entropy-chunk=abc", "fw.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(3, argv, &options, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
}

static void test_cli_bad_format_value(void)
{
    char *argv[] = {"binlens", "--format", "xyz", "fw.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(4, argv, &options, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
    assert(strstr(diag.message, "expected format") != NULL);
}

static void test_cli_multiple_input_files(void)
{
    char *argv[] = {"binlens", "first.hex", "second.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(3, argv, &options, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
    assert(strstr(diag.message, "only one input file") != NULL);
}

static void test_cli_missing_format_value(void)
{
    char *argv[] = {"binlens", "--format"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(2, argv, &options, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
}

static void test_cli_missing_base_value(void)
{
    char *argv[] = {"binlens", "--base", "fw.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(3, argv, &options, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
}

static void test_cli_missing_entropy_chunk_value(void)
{
    char *argv[] = {"binlens", "--entropy-chunk", "fw.hex"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(3, argv, &options, &diag) != 0);
    assert(diag.severity == BL_DIAG_ERROR);
}

static void test_cli_short_help_flag(void)
{
    char *argv[] = {"binlens", "-h"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(2, argv, &options, &diag) == 0);
    assert(options.show_help);
}

static void test_cli_help_without_input_ok(void)
{
    char *argv[] = {"binlens", "--help"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(2, argv, &options, &diag) == 0);
    assert(options.show_help);
}

static void test_cli_version_without_input_ok(void)
{
    char *argv[] = {"binlens", "--version"};
    BlCliOptions options;
    BlDiagnostic diag;
    assert(bl_cli_parse(2, argv, &options, &diag) == 0);
    assert(options.show_version);
}

int main(void)
{
    test_cli_options_init_defaults();
    test_cli_parse_help();
    test_cli_parse_version();
    test_cli_parse_verbose();
    test_cli_parse_heatmap();
    test_cli_parse_base_and_format();
    test_cli_parse_format_equals();
    test_cli_parse_base_equals();
    test_cli_parse_entropy_chunk_equals();
    test_cli_parse_entropy_chunk_flag();
    test_cli_parse_no_color();
    test_cli_missing_input_file();
    test_cli_unknown_option();
    test_cli_bad_base_not_a_number();
    test_cli_bad_base_equals_not_a_number();
    test_cli_bad_entropy_chunk_zero();
    test_cli_bad_entropy_chunk_not_a_number();
    test_cli_bad_format_value();
    test_cli_multiple_input_files();
    test_cli_missing_format_value();
    test_cli_missing_base_value();
    test_cli_missing_entropy_chunk_value();
    test_cli_short_help_flag();
    test_cli_help_without_input_ok();
    test_cli_version_without_input_ok();
    puts("test_cli: ok");
    return 0;
}