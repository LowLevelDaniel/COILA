#pragma once

#include <string>
#include <vector>
#include "parser/lexer.h"
#include "util/logger.h"
#include "util/source_location.h"

namespace coil {

/**
 * @brief Diagnostic severity
 */
enum DiagnosticSeverity {
    DIAG_NOTE,    // Informational note
    DIAG_WARNING, // Warning
    DIAG_ERROR,   // Error
    DIAG_FATAL    // Fatal error
};

/**
 * @brief Diagnostic message
 */
struct Diagnostic {
    DiagnosticSeverity severity; // Severity
    std::string message;         // Message
    SourceLocation location;     // Source location
    
    Diagnostic(DiagnosticSeverity sev, const std::string& msg, const SourceLocation& loc)
        : severity(sev), message(msg), location(loc) {}
};

/**
 * @brief Diagnostic engine
 * 
 * Collects and reports diagnostic messages.
 */
class DiagnosticEngine {
private:
    std::vector<Diagnostic> diagnostics; // Collected diagnostics
    bool hasErrors;                      // Has error diagnostics
    Logger* logger;                      // Logger for reporting
    
    /**
     * @brief Convert severity to log level
     * 
     * @param severity Diagnostic severity
     * @return Corresponding log level
     */
    static LogLevel severityToLogLevel(DiagnosticSeverity severity);
    
    /**
     * @brief Format a diagnostic message
     * 
     * @param diagnostic Diagnostic to format
     * @return Formatted message
     */
    static std::string formatDiagnostic(const Diagnostic& diagnostic);

public:
    /**
     * @brief Construct a new Diagnostic Engine
     * 
     * @param log Logger for reporting
     */
    explicit DiagnosticEngine(Logger* log = nullptr);
    
    /**
     * @brief Report a diagnostic
     * 
     * @param severity Severity
     * @param message Message
     * @param location Source location
     */
    void report(DiagnosticSeverity severity, const std::string& message, const SourceLocation& location);
    
    /**
     * @brief Report a note
     * 
     * @param message Message
     * @param location Source location
     */
    void note(const std::string& message, const SourceLocation& location);
    
    /**
     * @brief Report a warning
     * 
     * @param message Message
     * @param location Source location
     */
    void warning(const std::string& message, const SourceLocation& location);
    
    /**
     * @brief Report an error
     * 
     * @param message Message
     * @param location Source location
     */
    void error(const std::string& message, const SourceLocation& location);
    
    /**
     * @brief Report a fatal error
     * 
     * @param message Message
     * @param location Source location
     */
    void fatal(const std::string& message, const SourceLocation& location);
    
    /**
     * @brief Check if there are any diagnostics
     * 
     * @return true if there are diagnostics, false otherwise
     */
    bool hasDiagnostics() const;
    
    /**
     * @brief Check if there are any error diagnostics
     * 
     * @return true if there are error diagnostics, false otherwise
     */
    bool hasErrorDiagnostics() const;
    
    /**
     * @brief Get all diagnostics
     * 
     * @return Vector of diagnostics
     */
    const std::vector<Diagnostic>& getDiagnostics() const;
    
    /**
     * @brief Print all diagnostics
     */
    void printDiagnostics() const;
    
    /**
     * @brief Clear all diagnostics
     */
    void clear();
    
    /**
     * @brief Set the logger
     * 
     * @param log Logger
     */
    void setLogger(Logger* log);
};

} // namespace coil
