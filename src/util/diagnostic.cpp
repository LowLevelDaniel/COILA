#include "util/diagnostic.h"
#include "util/logger.h"
#include <sstream>
#include <iostream>

namespace coil {

LogLevel DiagnosticEngine::severityToLogLevel(DiagnosticSeverity severity) {
    switch (severity) {
        case DIAG_NOTE:    return LOG_INFO;
        case DIAG_WARNING: return LOG_WARNING;
        case DIAG_ERROR:   return LOG_ERROR;
        case DIAG_FATAL:   return LOG_FATAL;
        default:           return LOG_INFO;
    }
}

std::string DiagnosticEngine::formatDiagnostic(const Diagnostic& diagnostic) {
    std::ostringstream oss;
    
    // Format as: file:line:column: severity: message
    oss << diagnostic.location.toString() << ": ";
    
    switch (diagnostic.severity) {
        case DIAG_NOTE:    oss << "note: "; break;
        case DIAG_WARNING: oss << "warning: "; break;
        case DIAG_ERROR:   oss << "error: "; break;
        case DIAG_FATAL:   oss << "fatal error: "; break;
    }
    
    oss << diagnostic.message;
    
    return oss.str();
}

DiagnosticEngine::DiagnosticEngine(Logger* log)
    : hasErrors(false), logger(log) {
}

void DiagnosticEngine::report(DiagnosticSeverity severity, const std::string& message, const SourceLocation& location) {
    // Create diagnostic
    Diagnostic diagnostic(severity, message, location);
    
    // Add to collection
    diagnostics.push_back(diagnostic);
    
    // Update error flag
    if (severity >= DIAG_ERROR) {
        hasErrors = true;
    }
    
    // Log the diagnostic
    if (logger) {
        logger->log(severityToLogLevel(severity), formatDiagnostic(diagnostic));
    }
}

void DiagnosticEngine::note(const std::string& message, const SourceLocation& location) {
    report(DIAG_NOTE, message, location);
}

void DiagnosticEngine::warning(const std::string& message, const SourceLocation& location) {
    report(DIAG_WARNING, message, location);
}

void DiagnosticEngine::error(const std::string& message, const SourceLocation& location) {
    report(DIAG_ERROR, message, location);
}

void DiagnosticEngine::fatal(const std::string& message, const SourceLocation& location) {
    report(DIAG_FATAL, message, location);
}

bool DiagnosticEngine::hasDiagnostics() const {
    return !diagnostics.empty();
}

bool DiagnosticEngine::hasErrorDiagnostics() const {
    return hasErrors;
}

const std::vector<Diagnostic>& DiagnosticEngine::getDiagnostics() const {
    return diagnostics;
}

void DiagnosticEngine::printDiagnostics() const {
    for (const auto& diagnostic : diagnostics) {
        // Format and print the diagnostic
        std::string formatted = formatDiagnostic(diagnostic);
        
        // Use std::cerr for errors and fatal errors, std::cout for others
        if (diagnostic.severity >= DIAG_ERROR) {
            std::cerr << formatted << std::endl;
        } else {
            std::cout << formatted << std::endl;
        }
    }
}

void DiagnosticEngine::clear() {
    diagnostics.clear();
    hasErrors = false;
}

void DiagnosticEngine::setLogger(Logger* log) {
    logger = log;
}

} // namespace coil