#include "binlens/vector_table.h"

static void candidate_init(BlVectorTableCandidate *candidate)
{
    if (candidate == 0) {
        return;
    }

    candidate->table_address = 0;
    candidate->initial_stack_pointer = 0;
    candidate->reset_handler_raw = 0;
    candidate->reset_handler_address = 0;
    candidate->stack_pointer_valid = false;
    candidate->stack_pointer_aligned = false;
    candidate->reset_handler_valid = false;
    candidate->reset_handler_in_region = false;
    candidate->table_address_common = false;
    candidate->score = 0;
    candidate->confidence = BL_VECTOR_CONFIDENCE_NONE;
}

static int region_contains(const BlMemRegion *region, uint64_t address, size_t length)
{
    uint64_t end_address;

    if (region == 0 || length == 0) {
        return 0;
    }
    if ((uint64_t)(length - 1u) > UINT64_MAX - address) {
        return 0;
    }

    end_address = address + (uint64_t)length - 1u;
    return address >= region->start_address && end_address <= region->end_address;
}

static const BlMemRegion *find_region(const BlFirmwareImage *image,
                                      uint64_t address,
                                      size_t length)
{
    size_t i;

    if (image == 0) {
        return 0;
    }

    for (i = 0; i < image->region_count; i++) {
        if (region_contains(&image->regions[i], address, length)) {
            return &image->regions[i];
        }
    }

    return 0;
}

static uint32_t read_u32_le(const unsigned char *bytes)
{
    return ((uint32_t)bytes[0]) |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static BlVectorConfidence confidence_from_score(unsigned int score)
{
    if (score >= 6u) {
        return BL_VECTOR_CONFIDENCE_HIGH;
    }
    if (score >= 4u) {
        return BL_VECTOR_CONFIDENCE_MEDIUM;
    }
    if (score >= 2u) {
        return BL_VECTOR_CONFIDENCE_LOW;
    }
    return BL_VECTOR_CONFIDENCE_NONE;
}

static int is_common_table_address(uint64_t address)
{
    return address == 0x00000000u || address == 0x08000000u;
}

static void score_candidate(const BlFirmwareImage *image,
                            uint64_t table_address,
                            BlVectorTableCandidate *candidate)
{
    const BlMemRegion *table_region = find_region(image, table_address, 8u);
    size_t offset;

    candidate_init(candidate);
    if (table_region == 0) {
        return;
    }

    offset = (size_t)(table_address - table_region->start_address);
    candidate->table_address = table_address;
    candidate->initial_stack_pointer = read_u32_le(table_region->bytes + offset);
    candidate->reset_handler_raw = read_u32_le(table_region->bytes + offset + 4u);
    candidate->reset_handler_address = candidate->reset_handler_raw & ~1u;

    candidate->stack_pointer_valid =
        candidate->initial_stack_pointer >= 0x20000000u &&
        candidate->initial_stack_pointer <= 0x3FFFFFFFu;
    candidate->stack_pointer_aligned = (candidate->initial_stack_pointer & 0x3u) == 0u;
    candidate->reset_handler_valid = (candidate->reset_handler_raw & 1u) != 0u;
    candidate->reset_handler_in_region =
        find_region(image, candidate->reset_handler_address, 2u) != 0;
    candidate->table_address_common = is_common_table_address(table_address);

    if (candidate->stack_pointer_valid) {
        candidate->score += 2u;
        if (candidate->stack_pointer_aligned) {
            candidate->score += 1u;
        }
    }
    if (candidate->reset_handler_valid) {
        candidate->score += 2u;
    }
    if (candidate->reset_handler_in_region) {
        candidate->score += 2u;
    }
    if (candidate->table_address_common) {
        candidate->score += 1u;
    }

    candidate->confidence = confidence_from_score(candidate->score);
}

static void keep_better_candidate(BlVectorTableCandidate *best,
                                  const BlVectorTableCandidate *candidate)
{
    if (candidate->score > best->score ||
        (candidate->score == best->score &&
         candidate->confidence != BL_VECTOR_CONFIDENCE_NONE &&
         candidate->table_address < best->table_address)) {
        *best = *candidate;
    }
}

/* Detects a likely Cortex-M vector table candidate in a reconstructed image. */
int bl_vector_table_detect(const BlFirmwareImage *image,
                           BlVectorTableCandidate *candidate)
{
    BlVectorTableCandidate best;
    BlVectorTableCandidate current;
    size_t i;

    candidate_init(&best);

    if (image == 0 || candidate == 0 || image->region_count == 0) {
        if (candidate != 0) {
            candidate_init(candidate);
        }
        return 0;
    }

    score_candidate(image, 0x00000000u, &current);
    keep_better_candidate(&best, &current);

    score_candidate(image, 0x08000000u, &current);
    keep_better_candidate(&best, &current);

    for (i = 0; i < image->region_count; i++) {
        score_candidate(image, image->regions[i].start_address, &current);
        keep_better_candidate(&best, &current);
    }

    *candidate = best;
    return 0;
}

/* Returns a readable name for a vector table confidence value. */
const char *bl_vector_confidence_name(BlVectorConfidence confidence)
{
    switch (confidence) {
    case BL_VECTOR_CONFIDENCE_NONE:
        return "none";
    case BL_VECTOR_CONFIDENCE_LOW:
        return "low";
    case BL_VECTOR_CONFIDENCE_MEDIUM:
        return "medium";
    case BL_VECTOR_CONFIDENCE_HIGH:
        return "high";
    default:
        return "unknown";
    }
}
