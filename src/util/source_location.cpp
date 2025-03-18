#include "util/source_location.h"
#include <sstream>

namespace coil {

std::string SourceLocation::toString() const {
    std::ostringstream oss;
    oss << filename << ":" << line << ":" << column;
    return oss.str();
}

} // namespace coil