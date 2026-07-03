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

/* Simple memory range description. */
typedef struct BlMemRange {
    uint64_t start_address;
    uint64_t end_address;
    size_t length;
} BlMemRange;

/* Complete firmware image made up of one or more loaded chunks. */
typedef struct BlFirmwareImage {
    char source_path[BL_SOURCE_PATH_SIZE];
    BlInputFormat input_format;
    BlSourceChunk *chunks;
    size_t chunk_count;
    size_t chunk_capacity;
    size_t region_count;
    size_t gap_count;
    size_t overlap_count;
    size_t total_loaded_bytes;
    uint64_t address_start;
    uint64_t address_end;
} BlFirmwareImage;

/* Initializes an empty firmware image. */
void bl_firmware_image_init(BlFirmwareImage *image);

/* Frees memory owned by the firmware image. */
void bl_firmware_image_free(BlFirmwareImage *image);

/* Stores the orignal source file path for the image. */
void bl_firmware_image_set_source(BlFirmwareImage *image, const char *source_path);

/* Adds a new loaded data chunk to the firmware image. */
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

/* Returns a readable name for an input format.*/
const char *bl_input_format_name(BlInputFormat format);

#endif
