#ifndef COIL_UTIL_SOURCE_LOCATION_H
#define COIL_UTIL_SOURCE_LOCATION_H

#include <string>

namespace coil {

/**
 * @brief Token position in source code
 */
struct SourceLocation {
    std::string filename;  // Source filename
    int line;              // Line number (1-based)
    int column;            // Column number (1-based)
    
    SourceLocation(const std::string& file = "", int l = 1, int c = 1)
        : filename(file), line(l), column(c) {}
        
    std::string toString() const;
};

} // namespace coil

#endif // COIL_UTIL_SOURCE_LOCATION_H