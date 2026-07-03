#ifndef BINLENS_FORMAT_H
#define BINLENS_FORMAT_H

#include <stddef.h>
#include <stdint.h>

/* Formats an address into a readable string. */
void bl_format_address(uint64_t address, char *buffer, size_t buffer_size);

/* Formats a byte count into a human-readable size string. */
void bl_format_size(size_t bytes, char *buffer, size_t buffer_size);

#endif
