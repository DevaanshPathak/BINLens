#include "binlens/format.h"

#include <stdio.h>

/* Formats an address as an uppercase hexadecimal string. */
void bl_format_address(uint64_t address, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return;
    }

    snprintf(buffer, buffer_size, "0x%08llX", (unsigned long long)address);
}

/* Formats a byte count using B, KiB, or MiB units. */
void bl_format_size(size_t bytes, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return;
    }

    if (bytes >= 1024u * 1024u) {
        snprintf(buffer, buffer_size, "%.1f MiB", (double)bytes / (1024.0 * 1024.0));
    } else if (bytes >= 1024u) {
        snprintf(buffer, buffer_size, "%.1f KiB", (double)bytes / 1024.0);
    } else {
        snprintf(buffer, buffer_size, "%zu B", bytes);
    }
}
