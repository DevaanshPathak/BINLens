#include "binlens/diagnostic.h"

#include <stdarg.h>
#include <stdio.h>

/* Resets a diagnostic to an empty state. */
void bl_diag_clear(BlDiagnostic *diag)
{
    if (diag == NULL) {
        return;
    }

    diag->severity = BL_DIAG_NONE;
    diag->message[0] = '\0';
}

/* Sets the diagnostics severity and formatted message. */
void bl_diag_set(BlDiagnostic *diag,
                 BlDiagnosticSeverity severity,
                 const char *fmt,
                 ...)
{
    va_list args;

    if (diag == NULL) {
        return;
    }

    diag->severity = severity;

    if (fmt == NULL) {
        diag->message[0] = '\0';
        return;
    }

    va_start(args, fmt);
    vsnprintf(diag->message, sizeof(diag->message), fmt, args);
    va_end(args);
}

/* Returns a readable name for a diagnostic severity. */
const char *bl_diag_severity_name(BlDiagnosticSeverity severity)
{
    switch (severity) {
    case BL_DIAG_NONE:
        return "none";
    case BL_DIAG_WARNING:
        return "warning";
    case BL_DIAG_ERROR:
        return "error";
    default:
        return "unknown";
    }
}
