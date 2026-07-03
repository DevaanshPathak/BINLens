#include "binlens/memmap.h"

/* Reconstructs memory map details for a firmware image. */
int bl_memmap_reconstruct(BlFirmwareImage *image, BlDiagnostic *diag)
{
    (void)image;
    bl_diag_set(diag, BL_DIAG_ERROR, "memory map reconstruction is not implemented yet");
    return -1;
}
