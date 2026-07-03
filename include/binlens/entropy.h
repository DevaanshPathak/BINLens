#ifndef BINLENS_ENTROPY_H
#define BINLENS_ENTROPY_H

#include <stddef.h>

/**
* Calculates the Shannon entropy of a given byte buffer.
*
* Shannon entropy measures how random or information-dense the input data is.
* For binary data, the result typically ranges from 0.0 to 8.0, where lower
* values indicate repetitive or predictable data, and higher values indicate
* compressed, encrypted, or highly random-looking data.
*
* @param bytes  Pointer to the byte buffer to analyze.
* @param length Number of bytes in the buffer
*
* @return Shannon entropy value for the provided data.
*/
double bl_entropy_shannon(const unsigned char *bytes, size_t length);

/**
* Converts an entropy value into a heatmap display symbol.
*
* This is used by CLI output to visually represent entropy levels across
* chunks of firmware or binary data.
*
* @param entropy Shannon entropy value to convert.
*
* @return Character symbol representing the entropy level.
*/
char bl_entropy_heatmap_symbol(double entropy);

#endif
