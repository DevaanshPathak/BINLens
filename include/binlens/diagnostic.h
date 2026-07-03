#ifndef BINLENS_DIAGNOSTIC_H
#define BINLENS_DIAGNOSTIC_H

/**
* Represents the severity level of a diagnostic message.
*
* Diagnostics are used to report warnings and errors during parsing,
* loading, analysis, or other BINLens operations.
*/

typedef enum BlDiagnosticSeverity {
    /** No diagnostic message is currently set. */
    BL_DIAG_NONE = 0,
    /** A non-fatal warning occured. */
    BL_DIAG_WARNING,
    /** A fatal or operation-stopping error occurred. */
    BL_DIAG_ERROR
} BlDiagnosticSeverity;


/**
* Stores diagnostic information for an operation.
*
* A diagnostic contains both a severity level and a human-readable message.
* Functions can use this structure to report errors or warnings without
* printing directly to stdout or stderr.
*/

typedef struct BlDiagnostic {
    /** Severity level of the current diagnostic message. */
    BlDiagnosticSeverity severity;
    
    /** Human-readable diagnostic message. */
    char message[512];
} BlDiagnostic;

/** 
* Clears a diagnostic object.
*
* Resets the severity to BL_DIAG_NONE and clears the message buffer.
*
* @param diag Diagnostic object to clear.
*/
void bl_diag_clear(BlDiagnostic *diag);

/**
* Sets a diagnostic message with a severity level.
*
* The message is formatted using printf-style formatting rules.
* If diag is NULL, the function should safely do nothing.
*
* @param diag     Diagnostic object to update.
* @param severity Severity level to assign.
* @param fml      printf-style format string for the message.
* @param ...      Additional arguments used by the format string.
*/
void bl_diag_set(BlDiagnostic *diag,
                 BlDiagnosticSeverity severity,
                 const char *fmt,
                 ...);

/**
* Returns a readable name for a diagnostic severity value.
*
* This is useful when printing diagnostics in CLI outputs or logs.
*
* @param severity Diagnostic severity value.
*
* @return String name for the severity.
*/
const char *bl_diag_severity_name(BlDiagnosticSeverity severity);

#endif
