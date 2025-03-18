#include "core/variable.h"
#include <sstream>

namespace coil {

Variable::Variable(uint8_t vId, uint16_t vType, uint8_t vStorage, 
                   const std::string& vName)
    : varId(vId), storageClass(vStorage), typeId(vType), name(vName) {
}

uint8_t Variable::getVarId() const {
    return varId;
}

uint8_t Variable::getStorageClass() const {
    return storageClass;
}

uint16_t Variable::getTypeId() const {
    return typeId;
}

const std::string& Variable::getName() const {
    return name;
}

void Variable::setName(const std::string& vName) {
    name = vName;
}

void Variable::setInitialValue(const std::vector<uint8_t>& value) {
    initialValue = value;
}

const std::vector<uint8_t>& Variable::getInitialValue() const {
    return initialValue;
}

bool Variable::hasInitialValue() const {
    return !initialValue.empty();
}

std::string Variable::toString() const {
    std::ostringstream oss;
    
    // Add variable ID
    oss << getIdString();
    
    // Add name if present
    if (!name.empty()) {
        oss << " (" << name << ")";
    }
    
    // Add type
    oss << " : " << Type::fromBasicType(typeId).toString();
    
    // Add storage class
    oss << " [";
    switch (storageClass) {
        case STORAGE_AUTO:     oss << "auto"; break;
        case STORAGE_REGISTER: oss << "register"; break;
        case STORAGE_STACK:    oss << "stack"; break;
        case STORAGE_STATIC:   oss << "static"; break;
        case STORAGE_THREAD:   oss << "thread"; break;
        case STORAGE_GLOBAL:   oss << "global"; break;
        default:               oss << "unknown"; break;
    }
    oss << "]";
    
    return oss.str();
}

std::string Variable::getIdString() const {
    std::ostringstream oss;
    oss << "$" << static_cast<int>(varId);
    return oss.str();
}

} // namespace coil