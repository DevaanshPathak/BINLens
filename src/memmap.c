#include "binlens/memmap.h"

#include "binlens/entropy.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct RegionBuilder { /* Temporary builder used to assemble one contiguous reconstructed memory region. */
    uint64_t start_address;
    uint64_t end_address;
    unsigned char *bytes;
    size_t length;
    size_t capacity;
} RegionBuilder;

static int compare_chunks(const void *left, const void *right) /* Sorts chunks by address, then end address and source line for deterministic reconstruction. */
{
    const BlSourceChunk *a = (const BlSourceChunk *)left;
    const BlSourceChunk *b = (const BlSourceChunk *)right;

    if (a->start_address < b->start_address) {
        return -1;
    }
    if (a->start_address > b->start_address) {
        return 1;
    }
    if (a->end_address < b->end_address) {
        return -1;
    }
    if (a->end_address > b->end_address) {
        return 1;
    }
    if (a->origin_line < b->origin_line) {
        return -1;
    }
    if (a->origin_line > b->origin_line) {
        return 1;
    }
    return 0;
}

static int ensure_region_capacity(BlFirmwareImage *image, BlDiagnostic *diag) /* Ensures the reconstructed region table has room for one more entry. */
{
    if (image->region_count == image->region_capacity) {
        size_t next_capacity = image->region_capacity == 0 ? 4 : image->region_capacity * 2;
        BlMemRegion *next_regions;

        if (next_capacity < image->region_capacity) {
            bl_diag_set(diag, BL_DIAG_ERROR, "region table capacity overflow");
            return -1;
        }

        next_regions = (BlMemRegion *)realloc(image->regions,
                                              next_capacity * sizeof(*next_regions));
        if (next_regions == NULL) {
            bl_diag_set(diag, BL_DIAG_ERROR, "out of memory while growing region table");
            return -1;
        }

        image->regions = next_regions;
        image->region_capacity = next_capacity;
    }

    return 0;
}

static int ensure_gap_capacity(BlFirmwareImage *image, BlDiagnostic *diag) /* Ensures the gap table has room for one more unloaded address range. */
{
    if (image->gap_count == image->gap_capacity) {
        size_t next_capacity = image->gap_capacity == 0 ? 4 : image->gap_capacity * 2;
        BlMemGap *next_gaps;

        if (next_capacity < image->gap_capacity) {
            bl_diag_set(diag, BL_DIAG_ERROR, "gap table capacity overflow");
            return -1;
        }

        next_gaps = (BlMemGap *)realloc(image->gaps, next_capacity * sizeof(*next_gaps));
        if (next_gaps == NULL) {
            bl_diag_set(diag, BL_DIAG_ERROR, "out of memory while growing gap table");
            return -1;
        }

        image->gaps = next_gaps;
        image->gap_capacity = next_capacity;
    }

    return 0;
}

static int ensure_overlap_capacity(BlFirmwareImage *image, BlDiagnostic *diag) /* Ensures the overlap table has room for one more overlap diagnostic. */
{
    if (image->overlap_count == image->overlap_capacity) {
        size_t next_capacity = image->overlap_capacity == 0 ? 4 : image->overlap_capacity * 2;
        BlMemOverlap *next_overlaps;

        if (next_capacity < image->overlap_capacity) {
            bl_diag_set(diag, BL_DIAG_ERROR, "overlap table capacity overflow");
            return -1;
        }

        next_overlaps = (BlMemOverlap *)realloc(image->overlaps,
                                                next_capacity * sizeof(*next_overlaps));
        if (next_overlaps == NULL) {
            bl_diag_set(diag, BL_DIAG_ERROR, "out of memory while growing overlap table");
            return -1;
        }

        image->overlaps = next_overlaps;
        image->overlap_capacity = next_capacity;
    }

    return 0;
}

static int add_gap(BlFirmwareImage *image, /* Records an unloaded inclusive address range between two reconstructed regions. */
                   uint64_t start_address,
                   uint64_t end_address,
                   BlDiagnostic *diag)
{
    BlMemGap *gap;

    if (ensure_gap_capacity(image, diag) != 0) {
        return -1;
    }

    gap = &image->gaps[image->gap_count++];
    gap->start_address = start_address;
    gap->end_address = end_address;
    gap->length = end_address - start_address + 1u;
    return 0;
}

static int add_overlap(BlFirmwareImage *image, /* Records an overlap between two source chunks, preserving chunk indexes and source lines. */
                       size_t first_index,
                       size_t second_index,
                       uint64_t start_address,
                       uint64_t end_address,
                       BlOverlapKind kind,
                       BlDiagnostic *diag)
{
    BlMemOverlap *overlap;
    uint64_t length = end_address - start_address + 1u;

    if (length > (uint64_t)SIZE_MAX) { /* Reject overlaps that cannot be represented safely as a platform size_t length. */
        bl_diag_set(diag, BL_DIAG_ERROR, "overlap length exceeds platform size limit");
        return -1;
    }
    if (ensure_overlap_capacity(image, diag) != 0) {
        return -1;
    }

    overlap = &image->overlaps[image->overlap_count++];
    overlap->start_address = start_address;
    overlap->end_address = end_address;
    overlap->length = (size_t)length;
    overlap->first_chunk_index = first_index;
    overlap->second_chunk_index = second_index;
    overlap->first_origin_line = image->chunks[first_index].origin_line;
    overlap->second_origin_line = image->chunks[second_index].origin_line;
    overlap->kind = kind;
    return 0;
}

static int add_region_take(BlFirmwareImage *image, RegionBuilder *builder, BlDiagnostic *diag) /* Transfers the builder buffer into the image as a finalized reconstructed region.  */
{
    BlMemRegion *region;

    if (builder->length == 0) {
        return 0;
    }
    if (ensure_region_capacity(image, diag) != 0) {
        return -1;
    }

    region = &image->regions[image->region_count++];
    region->start_address = builder->start_address;
    region->end_address = builder->end_address;
    region->length = builder->length;
    region->bytes = builder->bytes;
    region->entropy = bl_entropy_shannon(builder->bytes, builder->length); /* Store entropy once per region so report generation does not need to recompute it. */

    builder->bytes = NULL; /* Ownership of builder -> bytes has moved to the region entry. */
    builder->length = 0;
    builder->capacity = 0;
    return 0;
}

static int builder_init_from_chunk(RegionBuilder *builder, /* Starts a region builder from a source chunk by copying its bytes. */
                                   const BlSourceChunk *chunk,
                                   BlDiagnostic *diag)
{
    builder->start_address = chunk->start_address;
    builder->end_address = chunk->end_address;
    builder->length = chunk->length;
    builder->capacity = chunk->length;
    builder->bytes = NULL;

    if (chunk->length == 0) {
        return 0;
    }

    builder->bytes = (unsigned char *)malloc(chunk->length);
    if (builder->bytes == NULL) {
        bl_diag_set(diag, BL_DIAG_ERROR, "out of memory while creating memory region");
        return -1;
    }

    memcpy(builder->bytes, chunk->bytes, chunk->length);
    return 0;
}

static int builder_append(RegionBuilder *builder, /* Appends bytes to the current region builder, growing its buffer if needed. */
                          const unsigned char *bytes,
                          size_t length,
                          BlDiagnostic *diag)
{
    if (length == 0) {
        return 0;
    }
    if (length > SIZE_MAX - builder->length) {
        bl_diag_set(diag, BL_DIAG_ERROR, "memory region length exceeds platform size limit");
        return -1;
    }

    if (builder->length + length > builder->capacity) {
        size_t next_capacity = builder->capacity == 0 ? length : builder->capacity;
        unsigned char *next_bytes;

        while (next_capacity < builder->length + length) { /* Grow geometrically, but fall back to the exact required size on overflow risk. */
            size_t doubled = next_capacity * 2;
            if (doubled <= next_capacity) {
                next_capacity = builder->length + length;
                break;
            }
            next_capacity = doubled;
        }

        next_bytes = (unsigned char *)realloc(builder->bytes, next_capacity);
        if (next_bytes == NULL) {
            bl_diag_set(diag, BL_DIAG_ERROR, "out of memory while extending memory region");
            return -1;
        }

        builder->bytes = next_bytes;
        builder->capacity = next_capacity;
    }

    memcpy(builder->bytes + builder->length, bytes, length);
    builder->length += length;
    return 0;
}

static int append_non_overlapping_tail(RegionBuilder *builder, /* Appends only the non-overlapping suffix of a chunk already merged into a region. */
                                       const BlSourceChunk *chunk,
                                       BlDiagnostic *diag)
{
    uint64_t tail_start;
    uint64_t offset;
    uint64_t tail_length;

    if (chunk->end_address <= builder->end_address) {
        return 0;
    }

    tail_start = builder->end_address + 1u; /* The tail begins immediately after the current region's inclusive end address. */
    offset = tail_start - chunk->start_address;
    tail_length = chunk->end_address - tail_start + 1u;

    if (offset > (uint64_t)SIZE_MAX || tail_length > (uint64_t)SIZE_MAX) {
        bl_diag_set(diag, BL_DIAG_ERROR, "memory region tail exceeds platform size limit");
        return -1;
    }

    if (builder_append(builder,
                       chunk->bytes + (size_t)offset,
                       (size_t)tail_length,
                       diag) != 0) {
        return -1;
    }

    builder->end_address = chunk->end_address;
    return 0;
}

static int chunks_are_conflicting(const BlSourceChunk *first, /* Compares overlapping byte ranges to classify them as indetical or conflicting. */
                                  const BlSourceChunk *second,
                                  uint64_t start_address,
                                  uint64_t end_address,
                                  BlDiagnostic *diag,
                                  int *conflicting)
{
    uint64_t first_offset = start_address - first->start_address;
    uint64_t second_offset = start_address - second->start_address;
    uint64_t length = end_address - start_address + 1u;
    size_t i;

    if (first_offset > (uint64_t)SIZE_MAX ||
        second_offset > (uint64_t)SIZE_MAX ||
        length > (uint64_t)SIZE_MAX) {
        bl_diag_set(diag, BL_DIAG_ERROR, "overlap comparison exceeds platform size limit");
        return -1;
    }

    *conflicting = 0;
    for (i = 0; i < (size_t)length; i++) {
        if (first->bytes[(size_t)first_offset + i] !=
            second->bytes[(size_t)second_offset + i]) {
            *conflicting = 1;
            break;
        }
    }

    return 0;
}

static int detect_overlaps(BlFirmwareImage *image, BlDiagnostic *diag) /* Finds all parawise chunk overlaps after chunks have been sorted by start address. */
{
    size_t i;

    for (i = 0; i < image->chunk_count; i++) {
        size_t j;

        for (j = i + 1u; /* Stop scanning once later chunks start beyond the chunk's end address. */
             j < image->chunk_count && image->chunks[j].start_address <= image->chunks[i].end_address;
             j++) {
            uint64_t start = image->chunks[i].start_address > image->chunks[j].start_address
                                 ? image->chunks[i].start_address
                                 : image->chunks[j].start_address;
            uint64_t end = image->chunks[i].end_address < image->chunks[j].end_address
                               ? image->chunks[i].end_address
                               : image->chunks[j].end_address;
            int conflicting = 0;

            if (chunks_are_conflicting(&image->chunks[i],
                                       &image->chunks[j],
                                       start,
                                       end,
                                       diag,
                                       &conflicting) != 0) {
                return -1;
            }

            if (add_overlap(image,
                            i,
                            j,
                            start,
                            end,
                            conflicting ? BL_OVERLAP_CONFLICTING : BL_OVERLAP_IDENTICAL,
                            diag) != 0) {
                return -1;
            }
        }
    }

    return 0;
}

static int can_merge(uint64_t current_end, uint64_t next_start) /* Chunks can merge when they overlap or are directly adjacent. */
{
    return next_start <= current_end || (current_end != UINT64_MAX && next_start == current_end + 1u);
}

/* Reconstructs contiguous regions, gaps, overlaps and entropy from source chunks. */
int bl_memmap_reconstruct(BlFirmwareImage *image, BlDiagnostic *diag)
{
    RegionBuilder builder;
    size_t i;

    bl_diag_clear(diag);

    if (image == NULL) {
        bl_diag_set(diag, BL_DIAG_ERROR, "invalid memory map argument");
        return -1;
    }

    bl_firmware_image_clear_layout(image); /* Drop any previous derived layout before rebuilding it from the current chunks. */

    if (image->chunk_count == 0) {
        return 0;
    }

    qsort(image->chunks, image->chunk_count, sizeof(image->chunks[0]), compare_chunks);

    if (detect_overlaps(image, diag) != 0) {
        goto fail;
    }

    memset(&builder, 0, sizeof(builder));
    if (builder_init_from_chunk(&builder, &image->chunks[0], diag) != 0) {
        goto fail;
    }

    for (i = 1; i < image->chunk_count; i++) {
        const BlSourceChunk *chunk = &image->chunks[i];

        if (can_merge(builder.end_address, chunk->start_address)) { /* Merge overlapping or adjacent chunks into the current reconstructed region. */
            if (append_non_overlapping_tail(&builder, chunk, diag) != 0) {
                goto fail_builder;
            }
            continue;
        }

        if (add_region_take(image, &builder, diag) != 0) {
            goto fail_builder;
        }
        if (add_gap(image, image->regions[image->region_count - 1u].end_address + 1u, /* A gap exists between the finalized region and the next chunk start. */
                    chunk->start_address - 1u, diag) != 0) {
            goto fail;
        }
        if (builder_init_from_chunk(&builder, chunk, diag) != 0) {
            goto fail;
        }
    }

    if (add_region_take(image, &builder, diag) != 0) {
        goto fail_builder;
    }

    return 0;

fail_builder: /* Free an in-progress builder buffer before clearing partially built layout state. */
    free(builder.bytes);
fail:
    bl_firmware_image_clear_layout(image);
    return -1;
}
