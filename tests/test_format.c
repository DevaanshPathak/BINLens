#include "binlens/format.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_format_address_zero(void)
{
    char buffer[32];
    bl_format_address(0, buffer, sizeof(buffer));
    assert(strcmp(buffer, "0x00000000") == 0);
}

static void test_format_address_typical(void)
{
    char buffer[32];
    bl_format_address(0x08000000u, buffer, sizeof(buffer));
    assert(strcmp(buffer, "0x08000000") == 0);
}

static void test_format_address_large(void)
{
    char buffer[32];
    bl_format_address(0xFFFFFFFFu, buffer, sizeof(buffer));
    assert(strcmp(buffer, "0xFFFFFFFF") == 0);
}

static void test_format_address_null_buffer(void)
{
    bl_format_address(0x1000u, NULL, 0);
}

static void test_format_address_small_buffer(void)
{
    char buffer[4];
    bl_format_address(0x08000000u, buffer, sizeof(buffer));
    assert(buffer[0] != '\0');
}

static void test_format_size_bytes(void)
{
    char buffer[32];
    bl_format_size(0, buffer, sizeof(buffer));
    assert(strcmp(buffer, "0 B") == 0);
}

static void test_format_size_small(void)
{
    char buffer[32];
    bl_format_size(512, buffer, sizeof(buffer));
    assert(strcmp(buffer, "512 B") == 0);
}

static void test_format_size_kib(void)
{
    char buffer[32];
    bl_format_size(2048, buffer, sizeof(buffer));
    assert(strcmp(buffer, "2.0 KiB") == 0);
}

static void test_format_size_mib(void)
{
    char buffer[32];
    bl_format_size(3 * 1024 * 1024, buffer, sizeof(buffer));
    assert(strcmp(buffer, "3.0 MiB") == 0);
}

static void test_format_size_gib(void)
{
    char buffer[32];
    bl_format_size(4ull * 1024 * 1024 * 1024, buffer, sizeof(buffer));
    assert(strstr(buffer, "GiB") != NULL);
}

static void test_format_size_null_buffer(void)
{
    bl_format_size(100, NULL, 0);
}

static void test_format_u64_size(void)
{
    char buffer[32];
    bl_format_u64_size(1024, buffer, sizeof(buffer));
    assert(strcmp(buffer, "1.0 KiB") == 0);
}

static void test_format_u64_size_large(void)
{
    char buffer[32];
    bl_format_u64_size(1024ull * 1024 * 1024 * 1024, buffer, sizeof(buffer));
    assert(strstr(buffer, "GiB") != NULL);
}

static void test_format_u64_size_null_buffer(void)
{
    bl_format_u64_size(100, NULL, 0);
}

int main(void)
{
    test_format_address_zero();
    test_format_address_typical();
    test_format_address_large();
    test_format_address_null_buffer();
    test_format_address_small_buffer();
    test_format_size_bytes();
    test_format_size_small();
    test_format_size_kib();
    test_format_size_mib();
    test_format_size_gib();
    test_format_size_null_buffer();
    test_format_u64_size();
    test_format_u64_size_large();
    test_format_u64_size_null_buffer();
    puts("test_format: ok");
    return 0;
}