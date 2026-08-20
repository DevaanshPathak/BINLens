#include "binlens/diagnostic.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_diag_clear_resets_state(void)
{
    BlDiagnostic diag;
    diag.severity = BL_DIAG_ERROR;
    diag.message[0] = 'x';
    diag.message[1] = '\0';
    bl_diag_clear(&diag);
    assert(diag.severity == BL_DIAG_NONE);
    assert(diag.message[0] == '\0');
}

static void test_diag_clear_null_is_safe(void)
{
    bl_diag_clear(NULL);
}

static void test_diag_set_error(void)
{
    BlDiagnostic diag;
    bl_diag_clear(&diag);
    bl_diag_set(&diag, BL_DIAG_ERROR, "something went wrong");
    assert(diag.severity == BL_DIAG_ERROR);
    assert(strcmp(diag.message, "something went wrong") == 0);
}

static void test_diag_set_warning(void)
{
    BlDiagnostic diag;
    bl_diag_clear(&diag);
    bl_diag_set(&diag, BL_DIAG_WARNING, "be careful");
    assert(diag.severity == BL_DIAG_WARNING);
    assert(strcmp(diag.message, "be careful") == 0);
}

static void test_diag_set_formatted_message(void)
{
    BlDiagnostic diag;
    bl_diag_clear(&diag);
    bl_diag_set(&diag, BL_DIAG_ERROR, "line %d: %s", 42, "checksum error");
    assert(strstr(diag.message, "line 42") != NULL);
    assert(strstr(diag.message, "checksum error") != NULL);
}

static void test_diag_set_null_diag_is_safe(void)
{
    bl_diag_set(NULL, BL_DIAG_ERROR, "should not crash");
}

static void test_diag_set_null_fmt_is_safe(void)
{
    BlDiagnostic diag;
    bl_diag_clear(&diag);
    diag.message[0] = 'x';
    diag.message[1] = '\0';
    bl_diag_set(&diag, BL_DIAG_WARNING, NULL);
    assert(diag.severity == BL_DIAG_WARNING);
    assert(diag.message[0] == '\0');
}

static void test_diag_severity_name_none(void)
{
    assert(strcmp(bl_diag_severity_name(BL_DIAG_NONE), "none") == 0);
}

static void test_diag_severity_name_warning(void)
{
    assert(strcmp(bl_diag_severity_name(BL_DIAG_WARNING), "warning") == 0);
}

static void test_diag_severity_name_error(void)
{
    assert(strcmp(bl_diag_severity_name(BL_DIAG_ERROR), "error") == 0);
}

static void test_diag_severity_name_unknown(void)
{
    assert(strcmp(bl_diag_severity_name((BlDiagnosticSeverity)99), "unknown") == 0);
}

int main(void)
{
    test_diag_clear_resets_state();
    test_diag_clear_null_is_safe();
    test_diag_set_error();
    test_diag_set_warning();
    test_diag_set_formatted_message();
    test_diag_set_null_diag_is_safe();
    test_diag_set_null_fmt_is_safe();
    test_diag_severity_name_none();
    test_diag_severity_name_warning();
    test_diag_severity_name_error();
    test_diag_severity_name_unknown();
    puts("test_diagnostic: ok");
    return 0;
}