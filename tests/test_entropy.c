#include "binlens/entropy.h"

#include <assert.h>
#include <stdio.h>

static double abs_double(double value)
{
    return value < 0.0 ? -value : value;
}

static void assert_near(double actual, double expected)
{
    assert(abs_double(actual - expected) < 0.0001);
}

static void test_empty_and_constant_entropy(void)
{
    const unsigned char zeros[] = {0, 0, 0, 0, 0, 0, 0, 0};

    assert_near(bl_entropy_shannon(0, 0), 0.0);
    assert_near(bl_entropy_shannon(zeros, sizeof(zeros)), 0.0);
}

static void test_known_entropy_values(void)
{
    const unsigned char two_symbols[] = {0, 1, 0, 1};
    const unsigned char four_symbols[] = {0, 1, 2, 3};
    unsigned char all_bytes[256];
    size_t i;

    for (i = 0; i < sizeof(all_bytes); i++) {
        all_bytes[i] = (unsigned char)i;
    }

    assert_near(bl_entropy_shannon(two_symbols, sizeof(two_symbols)), 1.0);
    assert_near(bl_entropy_shannon(four_symbols, sizeof(four_symbols)), 2.0);
    assert_near(bl_entropy_shannon(all_bytes, sizeof(all_bytes)), 8.0);
}

static void test_heatmap_symbols(void)
{
    assert(bl_entropy_heatmap_symbol(0.0) == '.');
    assert(bl_entropy_heatmap_symbol(1.5) == ':');
    assert(bl_entropy_heatmap_symbol(3.0) == '-');
    assert(bl_entropy_heatmap_symbol(4.5) == '=');
    assert(bl_entropy_heatmap_symbol(6.0) == '+');
    assert(bl_entropy_heatmap_symbol(7.0) == '*');
    assert(bl_entropy_heatmap_symbol(7.5) == '#');
    assert(bl_entropy_heatmap_symbol(7.9) == '%');
}

int main(void)
{
    test_empty_and_constant_entropy();
    test_known_entropy_values();
    test_heatmap_symbols();
    puts("test_entropy: ok");
    return 0;
}
