#include "binary/section.h"
#include <algorithm>

namespace coil {

Section::Section(const std::string& name, uint32_t type, uint32_t flags, uint32_t targetId, uint32_t alignment)
    : name(name), type(type), flags(flags), targetId(targetId), address(0), alignment(alignment) {
}

const std::string& Section::getName() const {
    return name;
}

uint32_t Section::getType() const {
    return type;
}

uint32_t Section::getFlags() const {
    return flags;
}

uint32_t Section::getTargetId() const {
    return targetId;
}

uint64_t Section::getAddress() const {
    return address;
}

void Section::setAddress(uint64_t addr) {
    address = addr;
}

uint32_t Section::getAlignment() const {
    return alignment;
}

const std::vector<uint8_t>& Section::getData() const {
    return data;
}

size_t Section::getSize() const {
    return data.size();
}

uint64_t Section::addData(const std::vector<uint8_t>& newData) {
    uint64_t offset = data.size();
    data.insert(data.end(), newData.begin(), newData.end());
    return offset;
}

void Section::addRelocation(uint64_t offset, uint32_t symbolIndex, uint32_t type, int64_t addend, uint32_t targetId) {
    RelocationEntry reloc;
    reloc.offset = offset;
    reloc.symbol_index = symbolIndex;
    reloc.type = type;
    reloc.addend = addend;
    reloc.target_id = targetId;
    
    relocations.push_back(reloc);
}

const std::vector<RelocationEntry>& Section::getRelocations() const {
    return relocations;
}

uint64_t Section::addInstruction(std::unique_ptr<Instruction> instruction) {
    uint64_t offset = instructions.size();
    instructions.push_back(std::move(instruction));
    return offset;
}

const std::vector<std::unique_ptr<Instruction>>& Section::getInstructions() const {
    return instructions;
}

std::vector<uint8_t> Section::getBytes(uint64_t offset, size_t size) const {
    if (offset >= data.size() || offset + size > data.size()) {
        return std::vector<uint8_t>();
    }
    
    return std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + size);
}

uint64_t Section::fillZero(size_t size) {
    uint64_t offset = data.size();
    data.resize(data.size() + size, 0);
    return offset;
}

uint64_t Section::align(uint32_t alignmentValue) {
    if (alignmentValue == 0) {
        alignmentValue = 1;
    }
    
    uint64_t currentSize = data.size();
    uint64_t padding = (alignmentValue - (currentSize % alignmentValue)) % alignmentValue;
    
    if (padding > 0) {
        data.resize(currentSize + padding, 0);
    }
    
    return currentSize + padding;
}

void Section::finalize() {
    // Convert instructions to binary data
    std::vector<uint8_t> instructionData;
    
    for (const auto& instruction : instructions) {
        std::vector<uint8_t> encoded = instruction->encode();
        instructionData.insert(instructionData.end(), encoded.begin(), encoded.end());
    }
    
    // Append instruction data to section data
    if (!instructionData.empty()) {
        addData(instructionData);
    }
}

SectionEntry Section::createEntry(uint32_t nameOffset, uint64_t sectionOffset, uint32_t relocOffset) const {
    SectionEntry entry;
    entry.name_offset = nameOffset;
    entry.type = type;
    entry.flags = flags;
    entry.target_id = targetId;
    entry.address = address;
    entry.size = static_cast<uint64_t>(data.size());
    entry.offset = sectionOffset;
    entry.alignment = alignment;
    entry.relocation_count = static_cast<uint32_t>(relocations.size());
    entry.relocation_offset = relocOffset;
    
    return entry;
}

} // namespace coil