#include "binlens/bin_loader.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void set_open_error(BlDiagnostic *diag, const char *path) /* Builds a detalied fopen diagnostic, including cwd to make relative path failures easier to debug. */
{
    char cwd[256];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        bl_diag_set(diag,
                    BL_DIAG_ERROR,
                    "could not open raw binary file: %s (%s); current directory: %s",
                    path,
                    strerror(errno),
                    cwd);
    } else {
        bl_diag_set(diag,
                    BL_DIAG_ERROR,
                    "could not open raw binary file: %s (%s)",
                    path,
                    strerror(errno));
    }
}

/* Loads a raw binary file at the supplied base address as one contiguous firmware chunk. */
int bl_bin_load_file(const char *path,
                     uint64_t base_address,
                     BlFirmwareImage *image, /* Diagnostic output is required because loader errors are reported through this object. */
                     BlDiagnostic *diag)
{
    FILE *file;
    long file_size;
    unsigned char *bytes = NULL;

    bl_diag_clear(diag);

    if (path == NULL || image == NULL) {
        bl_diag_set(diag, BL_DIAG_ERROR, "invalid raw binary loader argument");
        return -1;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        set_open_error(diag, path);
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        bl_diag_set(diag, BL_DIAG_ERROR, "could not seek raw binary file: %s", path);
        goto fail;
    }

    file_size = ftell(file);
    if (file_size < 0) {
        bl_diag_set(diag, BL_DIAG_ERROR, "could not determine raw binary size: %s", path);
        goto fail;
    }
    if ((unsigned long)file_size > (unsigned long)SIZE_MAX) {
        bl_diag_set(diag, BL_DIAG_ERROR, "raw binary file is too large for this platform: %s", path);
        goto fail;
    }

    if (fseek(file, 0, SEEK_SET) != 0) { /* Record source metadata even for empty binaries so the image still identifies its origin. */
        bl_diag_set(diag, BL_DIAG_ERROR, "could not rewind raw binary file: %s", path);
        goto fail; /* Empty raw binaries are valid inputs but do not create firmware chunks. */
    }

    image->input_format = BL_FORMAT_RAW_BIN;
    bl_firmware_image_set_source(image, path);

    if (file_size == 0) {
        fclose(file);
        return 0;
    }

    bytes = (unsigned char *)malloc((size_t)file_size);
    if (bytes == NULL) {
        bl_diag_set(diag, BL_DIAG_ERROR, "out of memory while reading raw binary file");
        goto fail;
    }
    /* add_chunk copies the buffer into image-owned storage, so this temporary allocation can be freed after insertion. */
    if (fread(bytes, 1, (size_t)file_size, file) != (size_t)file_size) {
        bl_diag_set(diag, BL_DIAG_ERROR, "could not read raw binary file: %s", path);
        goto fail;
    }

    if (bl_firmware_image_add_chunk(image,
                                    base_address,
                                    bytes,
                                    (size_t)file_size,
                                    path,
                                    0,
                                    diag) != 0) {
        goto fail; /* Discard partially loaded image state so callers never observe an incomplete binary load. */
    }

    free(bytes);
    fclose(file);
    return 0;

fail:
    free(bytes);
    fclose(file);
    bl_firmware_image_free(image);
    return -1;
}
