#include "binary/symbol.h"

namespace coil {

Symbol::Symbol(const std::string& name, uint32_t sectionIndex, uint64_t value, 
               uint64_t size, uint16_t type, uint16_t flags, uint32_t targetId)
    : name(name), sectionIndex(sectionIndex), value(value), 
      size(size), type(type), flags(flags), targetId(targetId) {
}

const std::string& Symbol::getName() const {
    return name;
}

uint32_t Symbol::getSectionIndex() const {
    return sectionIndex;
}

uint64_t Symbol::getValue() const {
    return value;
}

void Symbol::setValue(uint64_t val) {
    value = val;
}

uint64_t Symbol::getSize() const {
    return size;
}

void Symbol::setSize(uint64_t s) {
    size = s;
}

uint16_t Symbol::getType() const {
    return type;
}

uint16_t Symbol::getFlags() const {
    return flags;
}

bool Symbol::hasFlag(uint16_t flag) const {
    return (flags & flag) != 0;
}

void Symbol::addFlag(uint16_t flag) {
    flags |= flag;
}

uint32_t Symbol::getTargetId() const {
    return targetId;
}

bool Symbol::isGlobal() const {
    return hasFlag(SYMBOL_FLAG_GLOBAL);
}

bool Symbol::isUndefined() const {
    return hasFlag(SYMBOL_FLAG_UNDEFINED);
}

bool Symbol::isFunction() const {
    return type == SYMBOL_FUNCTION;
}

SymbolEntry Symbol::createEntry(uint32_t nameOffset) const {
    SymbolEntry entry;
    entry.name_offset = nameOffset;
    entry.section_index = sectionIndex;
    entry.value = value;
    entry.size = size;
    entry.type = type;
    entry.flags = flags;
    entry.target_id = targetId;
    
    return entry;
}

} // namespace coil