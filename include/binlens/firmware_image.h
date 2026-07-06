#ifndef BINLENS_FIRMWARE_IMAGE_H
#define BINLENS_FIRMWARE_IMAGE_H

#include <stddef.h>
#include <stdint.h>

#include "binlens/diagnostic.h"

/* Maximum stored path length for source/origin files. */
#define BL_SOURCE_PATH_SIZE 512u

/* Supported firmware input formats.*/
typedef enum BlInputFormat {
    BL_FORMAT_AUTO = 0,
    BL_FORMAT_INTEL_HEX,
    BL_FORMAT_RAW_BIN
} BlInputFormat;

/* A loaded block of firmware data with its address range and source info. */
typedef struct BlSourceChunk {
    uint64_t start_address;
    uint64_t end_address;
    size_t length;
    unsigned char *bytes;
    char origin_path[BL_SOURCE_PATH_SIZE];
    size_t origin_line;
} BlSourceChunk;

/* Address range descriptor used for lightweight layout summaries. */
typedef struct BlMemRange {
    uint64_t start_address;
    uint64_t end_address;
    uint64_t length;
} BlMemRange;
/* Reconstructed contiguous firmware region build from loaded chunks. */
typedef struct BlMemRegion {
    uint64_t start_address;
    uint64_t end_address;
    size_t length;
    unsigned char *bytes;
    double entropy;
} BlMemRegion;
/* Empty address span between reconstructed firmware regions.*/
typedef struct BlMemGap {
    uint64_t start_address;
    uint64_t end_address;
    uint64_t length;
} BlMemGap;
/* Describes whether overlapping chunks contain matching or conflicting bytes. */
typedef enum BlOverlapKind {
    BL_OVERLAP_IDENTICAL = 0,
    BL_OVERLAP_CONFLICTING
} BlOverlapKind;
/* Overlap record linking an address conflict back to the source chunks and lines. */
typedef struct BlMemOverlap {
    uint64_t start_address;
    uint64_t end_address;
    size_t length;
    size_t first_chunk_index;
    size_t second_chunk_index;
    size_t first_origin_line;
    size_t second_origin_line;
    BlOverlapKind kind;
} BlMemOverlap;

/* Complete firmware image made up of one or more loaded chunks. */
typedef struct BlFirmwareImage {
    char source_path[BL_SOURCE_PATH_SIZE];
    BlInputFormat input_format;
    BlSourceChunk *chunks;
    size_t chunk_count;
    size_t chunk_capacity;
    BlMemRegion *regions;
    size_t region_count;
    size_t region_capacity;
    BlMemGap *gaps;
    size_t gap_count;
    size_t gap_capacity;
    BlMemOverlap *overlaps;
    size_t overlap_count;
    size_t overlap_capacity;
    size_t total_loaded_bytes; /* Raw number of bytes loaded from source chunks, including duplicate overlap bytes. */
    uint64_t address_start; /* Lowest loaded firmware address, valid only when at least one chunk exists. */
    uint64_t address_end; /* Highest loaded firmware address, valid only when atleast one chunk exists. */
} BlFirmwareImage;

/* Initializes an empty firmware image. */
void bl_firmware_image_init(BlFirmwareImage *image);

/* Frees memory owned by the firmware image. */
void bl_firmware_image_free(BlFirmwareImage *image);

/* Clears reconstructed regions, gaps, and overlaps but preserves source chunks. */
void bl_firmware_image_clear_layout(BlFirmwareImage *image); /* Stores the orignal source file path for the image. */

/* Stores the orignal source file path for the image. */
void bl_firmware_image_set_source(BlFirmwareImage *image, const char *source_path);

/* Adds a chunk by copying the supplied bytes into image-owned storage. */
int bl_firmware_image_add_chunk(BlFirmwareImage *image,
                                uint64_t start_address,
                                const unsigned char *bytes,
                                size_t length,
                                const char *origin_path,
                                size_t origin_line,
                                BlDiagnostic *diag);

/* Returns a read-only chunk by index, or NULL if the index is invalid. */
const BlSourceChunk *bl_firmware_image_chunk_at(const BlFirmwareImage *image,
                                                size_t index);

/* Returns a read-only reconstructed region by index, or NULL if invalid. */
const BlMemRegion *bl_firmware_image_region_at(const BlFirmwareImage *image,
                                               size_t index);

/* Returns a read-only gap by index, or NULL if invalid. */
const BlMemGap *bl_firmware_image_gap_at(const BlFirmwareImage *image,
                                         size_t index);

/* Returns a read-only overlap by index, or NULL if invalid. */
const BlMemOverlap *bl_firmware_image_overlap_at(const BlFirmwareImage *image,
                                                 size_t index);

/* Returns a readable name for an input format.*/
const char *bl_input_format_name(BlInputFormat format);

/* Returns a readable name for an overlap kind. */
const char *bl_overlap_kind_name(BlOverlapKind kind);

#endif
