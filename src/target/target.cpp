#include "target/target.h"
#include "target/x86_64.h"
#include <algorithm>

namespace coil {

Target::Target(uint32_t targetId, uint8_t aClass, uint8_t aType, uint8_t wSize, uint8_t end, const std::string& targetName)
    : id(targetId), archClass(aClass), archType(aType), wordSize(wSize), endianness(end), features(0), extensions(0), defaultAbiId(0), name(targetName) {
}

uint32_t Target::getId() const {
    return id;
}

uint8_t Target::getArchClass() const {
    return archClass;
}

uint8_t Target::getArchType() const {
    return archType;
}

uint8_t Target::getWordSize() const {
    return wordSize;
}

uint8_t Target::getEndianness() const {
    return endianness;
}

uint32_t Target::getFeatures() const {
    return features;
}

void Target::setFeatures(uint32_t feat) {
    features = feat;
}

uint32_t Target::getExtensions() const {
    return extensions;
}

void Target::setExtensions(uint32_t ext) {
    extensions = ext;
}

void Target::addRegisterMapping(const RegisterMapping& mapping) {
    regMappings.push_back(mapping);
}

const std::vector<RegisterMapping>& Target::getRegisterMappings() const {
    return regMappings;
}

void Target::setDefaultAbiId(uint32_t abiId) {
    defaultAbiId = abiId;
}

uint32_t Target::getDefaultAbiId() const {
    return defaultAbiId;
}

const std::string& Target::getName() const {
    return name;
}

uint8_t Target::getPhysicalRegister(uint8_t vregId) const {
    // Find the mapping for the virtual register
    auto it = std::find_if(regMappings.begin(), regMappings.end(),
                           [vregId](const RegisterMapping& mapping) {
                               return mapping.vregId == vregId;
                           });
    
    if (it != regMappings.end()) {
        return it->pregId;
    }
    
    // No mapping found
    return 0xFF;
}

std::unique_ptr<Target> Target::createFromConfig(uint32_t targetId, const std::vector<uint8_t>& configData) {
    // TODO: Implement configuration-based target creation
    (void)configData; // Unused for now
    
    // Default to x86-64 for now
    return std::make_unique<X86_64Target>(targetId);
}

std::unique_ptr<Target> Target::createFromArchType(uint32_t targetId, uint8_t archType) {
    switch (archType) {
        case ARCH_X86_64:
            return std::make_unique<X86_64Target>(targetId);
        // Add other architecture types as needed
        default:
            // Unknown architecture type, default to x86-64
            return std::make_unique<X86_64Target>(targetId);
    }
}

} // namespace coil