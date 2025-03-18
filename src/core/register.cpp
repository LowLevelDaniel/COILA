#include "core/register.h"
#include <sstream>

namespace coil {

Register::Register(uint8_t rClass, uint8_t rId, uint8_t rSize, uint8_t rFlags, 
                   const std::string& rName)
    : regClass(rClass), regId(rId), regSize(rSize), flags(rFlags), name(rName) {
}

uint8_t Register::getRegClass() const {
    return regClass;
}

uint8_t Register::getRegId() const {
    return regId;
}

uint8_t Register::getRegSize() const {
    return regSize;
}

uint8_t Register::getFlags() const {
    return flags;
}

const std::string& Register::getName() const {
    return name;
}

void Register::setName(const std::string& rName) {
    name = rName;
}

std::string Register::getIdString() const {
    std::ostringstream oss;
    
    // Format based on register class
    switch (regClass) {
        case REG_GP:
            oss << "R" << static_cast<int>(regId);
            break;
        case REG_FP:
            oss << "F" << static_cast<int>(regId);
            break;
        case REG_VEC:
            oss << "V" << static_cast<int>(regId);
            break;
        case REG_SPECIAL:
            // Special registers have specific names
            switch (regId) {
                case REG_PC:        oss << "PC"; break;
                case REG_SP:        oss << "SP"; break;
                case REG_FRAME_PTR: oss << "FP"; break;
                case REG_FLAGS:     oss << "FLAGS"; break;
                case REG_LR:        oss << "LR"; break;
                default:            oss << "SR" << static_cast<int>(regId); break;
            }
            break;
        default:
            oss << "REG" << static_cast<int>(regId);
            break;
    }
    
    return oss.str();
}

} // namespace coil