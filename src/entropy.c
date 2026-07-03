#include "binlens/entropy.h"


/* Calculates the Shannon entropy of a byte sequence. */
double bl_entropy_shannon(const unsigned char *bytes, size_t length)
{
    (void)bytes;

    if (length == 0) {
        return 0.0;
    }

    return 0.0;
}

/* Maps an entropy value to an ASCII heatmap symbol. */
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
