#include "binlens/vector_table.h"

/* Detects a likely vector table candidate in the firmware image. */
int bl_vector_table_detect(const BlFirmwareImage *image,
                           BlVectorTableCandidate *candidate)
{
    /* Supress unused parameter warning until detection logic is implemented. */
    (void)image;

    if (candidate != 0) {
        candidate->table_address = 0;
        candidate->initial_stack_pointer = 0;
        candidate->reset_handler_raw = 0;
        candidate->reset_handler_address = 0;
        candidate->stack_pointer_valid = false;
        candidate->reset_handler_valid = false;
        candidate->confidence = BL_VECTOR_CONFIDENCE_NONE;
    }

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
