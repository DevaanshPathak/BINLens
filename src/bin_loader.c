#include "binlens/bin_loader.h"

int bl_bin_load_file(const char *path,
                     uint64_t base_address,
                     BlFirmwareImage *image,
                     BlDiagnostic *diag)
{
    /* Suppress unused-parameter warnings until the loader is implemented. */
    (void)path;
    (void)base_address;
    (void)image;

    /* Report that raw binary loading is currently unavailable. */
    bl_diag_set(diag, BL_DIAG_ERROR, "raw binary loader is not implemented yet");
    return -1;
}
