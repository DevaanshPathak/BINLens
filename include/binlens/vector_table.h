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

/* Possible ARM/Cortex-M vector table location and validation details. */
typedef struct BlVectorTableCandidate {
    uint64_t table_address;
    uint32_t initial_stack_pointer;
    uint32_t reset_handler_raw;
    uint32_t reset_handler_address;
    bool stack_pointer_valid;
    bool reset_handler_valid;
    BlVectorConfidence confidence;
} BlVectorTableCandidate;

/* Detects a likely vector table in the firmware image. */
int bl_vector_table_detect(const BlFirmwareImage *image,
                           BlVectorTableCandidate *candidate);

/* Returns a readable name for a vector table confidence level. */
const char *bl_vector_confidence_name(BlVectorConfidence confidence);

#endif
