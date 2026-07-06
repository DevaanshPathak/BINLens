#include "binlens/format.h"

#include <stdio.h>

/* Formats an address as a fixed-width uppercase hexadecimal string for display. */
void bl_format_address(uint64_t address, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return;
    }

    snprintf(buffer, buffer_size, "0x%08llX", (unsigned long long)address); /* Use snprintf so small buffers are safely truncated instead of overflowing. */
}

/* Delegates size_t formatting to the uint64_t formatter for consistent units. */
void bl_format_size(size_t bytes, char *buffer, size_t buffer_size)
{
    bl_format_u64_size((uint64_t)bytes, buffer, buffer_size);
}

/* Formats byte counts using binary units, scaling from B up to GiB. */
void bl_format_u64_size(uint64_t bytes, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return;
    }

    if (bytes >= 1024ull * 1024ull * 1024ull) { /* Choose the largest supported unit that keeps the displayed value readable. */
        snprintf(buffer, buffer_size, "%.1f GiB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
    } else if (bytes >= 1024ull * 1024ull) {
        snprintf(buffer, buffer_size, "%.1f MiB", (double)bytes / (1024.0 * 1024.0));
    } else if (bytes >= 1024ull) {
        snprintf(buffer, buffer_size, "%.1f KiB", (double)bytes / 1024.0);
    } else {
        snprintf(buffer, buffer_size, "%llu B", (unsigned long long)bytes);
    }
}
