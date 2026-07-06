#include "binlens/firmware_image.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Resets the image to an empty, reusable state with auto-detected input format. */
void bl_firmware_image_init(BlFirmwareImage *image)
{
    if (image == NULL) {
        return;
    }

    memset(image, 0, sizeof(*image));
    image->input_format = BL_FORMAT_AUTO;
}

/* Releases all image-owned memory, including chunks and reconstructed layout data. */
void bl_firmware_image_free(BlFirmwareImage *image)
{
    size_t i;

    if (image == NULL) {
        return;
    }

    bl_firmware_image_clear_layout(image); /* Clear derived layout first because regions own seperate byte buffers. */

    for (i = 0; i < image->chunk_count; i++) {
        free(image->chunks[i].bytes);
    }
    free(image->chunks);

    bl_firmware_image_init(image);
}

/* Frees reconstructed regions, gaps, and overlaps while preserving loaded source chunks. */
void bl_firmware_image_clear_layout(BlFirmwareImage *image)
{
    size_t i;

    if (image == NULL) {
        return;
    }

    for (i = 0; i < image->region_count; i++) {
        free(image->regions[i].bytes);
    }
    free(image->regions);
    free(image->gaps);
    free(image->overlaps);

    image->regions = NULL;
    image->region_count = 0;
    image->region_capacity = 0;
    image->gaps = NULL;
    image->gap_count = 0;
    image->gap_capacity = 0;
    image->overlaps = NULL;
    image->overlap_count = 0;
    image->overlap_capacity = 0;
}

/* Stores a bounded, null-terminated copy of the source file path. */
void bl_firmware_image_set_source(BlFirmwareImage *image, const char *source_path)
{
    if (image == NULL || source_path == NULL) {
        return;
    }

    strncpy(image->source_path, source_path, sizeof(image->source_path) - 1);
    image->source_path[sizeof(image->source_path) - 1] = '\0';
}

/* Adds a firmware chunk by copying caller-provided bytes into image-owned storage. */
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

    if ((uint64_t)(length - 1) > UINT64_MAX - start_address) { /* Reject chunks whose inclusive end address would overflow uint64_t. */
        bl_diag_set(diag, BL_DIAG_ERROR, "address range overflows 64-bit address space");
        return -1;
    }
    if (length > SIZE_MAX - image->total_loaded_bytes) { /* Keep total_loaded_bytes from wrapping when accumulating raw chunk sizes. */
        bl_diag_set(diag, BL_DIAG_ERROR, "loaded byte count overflows size limit");
        return -1;
    }

    if (image->chunk_count == image->chunk_capacity) { /* Grow the chunk table geometrically to keep repeated appends efficient. */
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

    copy = (unsigned char *)malloc(length); /* Copy bytes before storing the chunk so callers may free their input buffer safely. */
    if (copy == NULL) {
        bl_diag_set(diag, BL_DIAG_ERROR, "out of memory while copying firmware bytes");
        return -1;
    }
    memcpy(copy, bytes, length);

    end_address = start_address + (uint64_t)length - 1u; /* Chunk address ranges are inclusive, so end is start plus length minus one. */
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

    if (image->total_loaded_bytes == 0) { /* Maintain the overall loaded address bounds across all chunks. */
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

/* Returns a reconstructed region by index, or NULL if unavailable. */
const BlMemRegion *bl_firmware_image_region_at(const BlFirmwareImage *image,
                                               size_t index)
{
    if (image == NULL || index >= image->region_count) {
        return NULL;
    }

    return &image->regions[index];
}

/* Returns a memory gap by index, or NULL if unavailable. */
const BlMemGap *bl_firmware_image_gap_at(const BlFirmwareImage *image,
                                         size_t index)
{
    if (image == NULL || index >= image->gap_count) {
        return NULL;
    }

    return &image->gaps[index];
}

/* Returns an overlap diagnostic by index, or NULL if unavailable. */
const BlMemOverlap *bl_firmware_image_overlap_at(const BlFirmwareImage *image,
                                                 size_t index)
{
    if (image == NULL || index >= image->overlap_count) {
        return NULL;
    }

    return &image->overlaps[index];
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

/* Returns a readable overlap kind name. */
const char *bl_overlap_kind_name(BlOverlapKind kind)
{
    switch (kind) {
    case BL_OVERLAP_IDENTICAL:
        return "identical";
    case BL_OVERLAP_CONFLICTING:
        return "conflicting";
    default:
        return "unknown";
    }
}
