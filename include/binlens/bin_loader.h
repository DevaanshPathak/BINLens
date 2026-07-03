#ifndef BINLENS_BIN_LOADER_H
#define BINLENS_BIN_LOADER_H

#include <stdint.h>

#include "binlens/diagnostic.h"
#include "binlens/firmware_image.h"

/**
* Loads a raw binary firmware file into a firmware image
*
* The binary file is read from disk and mapped into the provided
* firmware image using the supplied base address. Since raw binary
* files do not contain embedded address metadata, the caller must
* provide the address where the binary should be considered loaded.
*
* Any load errors, file access issues, or validation problems are
* reported through the diagnostic object when provided.
*
* @param path         Path to the binary file to load
* @param base_address Base address to assign to the loaded binary data
* @param image        Firmware image object that receives the loaded data
* @param diag         Diagnostic object used for reporting errors or warnings
*
* @return 0 on success, or a non-zero value on failure.
*/

int bl_bin_load_file(const char *path,
                     uint64_t base_address,
                     BlFirmwareImage *image,
                     BlDiagnostic *diag);

#endif
