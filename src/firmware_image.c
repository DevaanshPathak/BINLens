#include "binlens/firmware_image.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Initializes a firmware image with default values. */
void bl_firmware_image_init(BlFirmwareImage *image)
{
    if (image == NULL) {
        return;
    }

    memset(image, 0, sizeof(*image));
    image->input_format = BL_FORMAT_AUTO;
}

/* Frees all chunks and resets the image. */
void bl_firmware_image_free(BlFirmwareImage *image)
{
    size_t i;

    if (image == NULL) {
        return;
    }

    for (i = 0; i < image->chunk_count; i++) {
        free(image->chunks[i].bytes);
    }
    free(image->chunks);

    bl_firmware_image_init(image);
}

/* Stores the source file path for the image. */
void bl_firmware_image_set_source(BlFirmwareImage *image, const char *source_path)
{
    if (image == NULL || source_path == NULL) {
        return;
    }

    strncpy(image->source_path, source_path, sizeof(image->source_path) - 1);
    image->source_path[sizeof(image->source_path) - 1] = '\0';
}

/* Adds a copied firmware data chunk to the image. */
int bl_firmware_image_add_chunk(BlFirmwareImage *image,
                                uint64_t start_address,
                                const unsigned char *bytes,
                                size_t length,
                                const char *origin_path,
                                size_t origin_line,
                                BlDiagnostic *diag)
{
    BlSourceChunk *chunk;
    unsigned char *copy;
    uint64_t end_address;

    if (image == NULL || (bytes == NULL && length > 0)) {
        bl_diag_set(diag, BL_DIAG_ERROR, "invalid firmware chunk");
        return -1;
    }

    if (length == 0) {
        return 0;
    }

    if ((uint64_t)(length - 1) > UINT64_MAX - start_address) {
        bl_diag_set(diag, BL_DIAG_ERROR, "address range overflows 64-bit address space");
        return -1;
    }
    if (length > SIZE_MAX - image->total_loaded_bytes) {
        bl_diag_set(diag, BL_DIAG_ERROR, "loaded byte count overflows size limit");
        return -1;
    }

    if (image->chunk_count == image->chunk_capacity) {
        size_t next_capacity = image->chunk_capacity == 0 ? 8 : image->chunk_capacity * 2;
        BlSourceChunk *next_chunks;

        if (next_capacity < image->chunk_capacity) {
            bl_diag_set(diag, BL_DIAG_ERROR, "chunk table capacity overflow");
            return -1;
        }

        next_chunks = (BlSourceChunk *)realloc(image->chunks,
                                               next_capacity * sizeof(*next_chunks));
        if (next_chunks == NULL) {
            bl_diag_set(diag, BL_DIAG_ERROR, "out of memory while growing chunk table");
            return -1;
        }

        image->chunks = next_chunks;
        image->chunk_capacity = next_capacity;
    }

    copy = (unsigned char *)malloc(length);
    if (copy == NULL) {
        bl_diag_set(diag, BL_DIAG_ERROR, "out of memory while copying firmware bytes");
        return -1;
    }
    memcpy(copy, bytes, length);

    end_address = start_address + (uint64_t)length - 1u;
    chunk = &image->chunks[image->chunk_count++];
    chunk->start_address = start_address;
    chunk->end_address = end_address;
    chunk->length = length;
    chunk->bytes = copy;
    chunk->origin_line = origin_line;
    chunk->origin_path[0] = '\0';
    if (origin_path != NULL) {
        strncpy(chunk->origin_path, origin_path, sizeof(chunk->origin_path) - 1);
        chunk->origin_path[sizeof(chunk->origin_path) - 1] = '\0';
    }

    if (image->total_loaded_bytes == 0) {
        image->address_start = start_address;
        image->address_end = end_address;
    } else {
        if (start_address < image->address_start) {
            image->address_start = start_address;
        }
        if (end_address > image->address_end) {
            image->address_end = end_address;
        }
    }

    image->total_loaded_bytes += length;
    return 0;
}

/* Returns a chunk by index, or NULL if unavailable. */
const BlSourceChunk *bl_firmware_image_chunk_at(const BlFirmwareImage *image,
                                                size_t index)
{
    if (image == NULL || index >= image->chunk_count) {
        return NULL;
    }

    return &image->chunks[index];
}

/* Returns a readable input format name. */
const char *bl_input_format_name(BlInputFormat format)
{
    switch (format) {
    case BL_FORMAT_AUTO:
        return "auto";
    case BL_FORMAT_INTEL_HEX:
        return "Intel HEX";
    case BL_FORMAT_RAW_BIN:
        return "raw binary";
    default:
        return "unknown";
    }
}
