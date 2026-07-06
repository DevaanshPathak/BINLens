#ifndef BINLENS_VECTOR_TABLE_H
#define BINLENS_VECTOR_TABLE_H

#include <stdbool.h>
#include <stdint.h>

#include "binlens/firmware_image.h"

/* Confidence level for a detected vector table candidate. */
typedef enum BlVectorConfidence {
    BL_VECTOR_CONFIDENCE_NONE = 0,
    BL_VECTOR_CONFIDENCE_LOW,
    BL_VECTOR_CONFIDENCE_MEDIUM,
    BL_VECTOR_CONFIDENCE_HIGH
} BlVectorConfidence;

/* ARM/Cortex-M vector table candidate with parsed entries and validation signals. */
typedef struct BlVectorTableCandidate {
    uint64_t table_address;
    uint32_t initial_stack_pointer;
    uint32_t reset_handler_raw; /* Raw reset vector value as stored in firmware, including Thumb-state bit. */
    uint32_t reset_handler_address; /* Normalized reset handler address with metadata bits cleared. */
    bool stack_pointer_valid;
    bool stack_pointer_aligned;
    bool reset_handler_valid;
    bool reset_handler_in_region;
    bool table_address_common; /* True when the table starts at a common vector table address. */
    unsigned int score; /* Heuristic score used to rank candidate tables before assigning confidence. */
    BlVectorConfidence confidence;
} BlVectorTableCandidate;

/* Scans the firmware image and returns the strongest vector table candidate. */
int bl_vector_table_detect(const BlFirmwareImage *image,
                           BlVectorTableCandidate *candidate);

/* Returns a readable name for a vector table confidence level. */
const char *bl_vector_confidence_name(BlVectorConfidence confidence);

#endif
