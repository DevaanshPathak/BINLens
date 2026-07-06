#include "binlens/entropy.h"

#define BL_LN2 0.69314718055994530942

static double bl_log2_approx(double value) /* Computes log2 without libm so entropy code does not depend on math library linkage. */
{
    int exponent = 0;
    double z;
    double term;
    double sum;
    unsigned int denominator;

    if (value <= 0.0) {
        return 0.0;
    }

    while (value >= 2.0) { /* Normalize value into [1,2) and track the power of two exponent seperately. */
        value *= 0.5;
        exponent++;
    }
    while (value < 1.0) {
        value *= 2.0;
        exponent--;
    }

    z = (value - 1.0) / (value + 1.0); /* Use the atanh-series form of ln(value) for stable approximation near 1. */
    term = z;
    sum = 0.0;
    denominator = 1;

    while (denominator <= 39u) { /* Truncate the odd-term series after 39 to balance accuracy and speed. */
        sum += term / (double)denominator;
        term *= z * z;
        denominator += 2u;
    }

    return (double)exponent + ((2.0 * sum) / BL_LN2);
}

/* Calculates Shannon entropy in bits per byte for a byte buffer. */
double bl_entropy_shannon(const unsigned char *bytes, size_t length)
{
    size_t counts[256] = {0}; /* Frequency table for every possible byte value. */
    size_t i;
    double entropy = 0.0;

    if (bytes == NULL || length == 0) {
        return 0.0;
    }

    for (i = 0; i < length; i++) {
        counts[bytes[i]]++;
    }

    for (i = 0; i < 256u; i++) {
        if (counts[i] != 0) {
            double probability = (double)counts[i] / (double)length;
            entropy -= probability * bl_log2_approx(probability);
        }
    }

    if (entropy < 0.000000001 && entropy > -0.000000001) { /* Clamp tiny floating-point noise around zero to an exact zero entropy value. */
        return 0.0;
    }

    return entropy;
}

/* Maps an entropy to progressively denser ASCII heatmap symbols. */
char bl_entropy_heatmap_symbol(double entropy)
{
    if (entropy < 1.0) {
        return '.';
    }
    if (entropy < 2.5) {
        return ':';
    }
    if (entropy < 4.0) {
        return '-';
    }
    if (entropy < 5.5) {
        return '=';
    }
    if (entropy < 6.5) {
        return '+';
    }
    if (entropy < 7.2) {
        return '*';
    }
    if (entropy < 7.8) {
        return '#';
    }
    return '%';
}
