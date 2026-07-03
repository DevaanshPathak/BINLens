#ifndef BINLENS_MEMMAP_H
#define BINLENS_MEMMAP_H

#include "binlens/diagnostic.h"
#include "binlens/firmware_image.h"

/* Reconstructs memory regions, gaps, and overlaps for a firmware image. */
int bl_memmap_reconstruct(BlFirmwareImage *image, BlDiagnostic *diag);

#endif
