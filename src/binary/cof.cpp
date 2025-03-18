#include "binary/cof.h"
#include "util/logger.h"
#include <ctime>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <random>

namespace coil {

CofFile::CofFile() {
    // Initialize header
    header.magic = COF_MAGIC;
    header.version_major = COF_VERSION_MAJOR;
    header.version_minor = COF_VERSION_MINOR;
    header.flags = 0;
    header.target_count = 0;
    header.section_count = 0;
    header.symbol_count = 0;
    header.string_table_size = 0;
    header.entry_point = 0;
    header.timestamp = static_cast<uint64_t>(std::time(nullptr));
    std::memset(header.uuid, 0, sizeof(header.uuid));
    
    // Generate a random UUID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    
    for (int i = 0; i < 16; i++) {
        header.uuid[i] = dist(gen);
    }
    
    // Set version 4 UUID (random)
    header.uuid[6] = (header.uuid[6] & 0x0F) | 0x40;
    header.uuid[8] = (header.uuid[8] & 0x3F) | 0x80;
    
    // Initialize header with default offsets
    header.header_size = sizeof(CofHeader);
    header.section_table_offset = 0;
    header.symbol_table_offset = 0;
    header.string_table_offset = 0;
    header.target_table_offset = 0;
    
    // Add empty string to string table (index 0)
    addString("");
}

uint32_t CofFile::addTarget(uint32_t archType, uint32_t features, const std::string& name) {
    uint32_t targetId = static_cast<uint32_t>(targets.size()) + 1;
    
    TargetEntry target;
    target.target_id = targetId;
    target.arch_type = archType;
    target.features = features;
    target.name_offset = addString(name);
    target.config_offset = 0; // No configuration data yet
    target.config_size = 0;
    
    targets.push_back(target);
    header.target_count = static_cast<uint32_t>(targets.size());
    
    return targetId;
}

Section& CofFile::addSection(const std::string& name, uint32_t type, uint32_t flags, uint32_t targetId) {
    auto section = std::make_unique<Section>(name, type, flags, targetId);
    Section& sectionRef = *section;
    
    sections.push_back(std::move(section));
    header.section_count = static_cast<uint32_t>(sections.size());
    
    return sectionRef;
}

uint32_t CofFile::addSymbol(const std::string& name, uint32_t sectionIndex, uint64_t value, 
                          uint64_t size, uint16_t type, uint16_t flags, uint32_t targetId) {
    uint32_t symbolIndex = static_cast<uint32_t>(symbols.size());
    
    auto symbol = std::make_unique<Symbol>(name, sectionIndex, value, size, type, flags, targetId);
    symbols.push_back(std::move(symbol));
    header.symbol_count = static_cast<uint32_t>(symbols.size());
    
    return symbolIndex;
}

uint32_t CofFile::addString(const std::string& str) {
    // Check if the string already exists in the string table
    auto it = stringTable.find(str);
    if (it != stringTable.end()) {
        return it->second;
    }
    
    // Add the string to the string table
    uint32_t offset = static_cast<uint32_t>(stringTableData.size());
    stringTable[str] = offset;
    
    // Append the string to the string table data (including null terminator)
    stringTableData.insert(stringTableData.end(), str.begin(), str.end());
    stringTableData.push_back(0);
    
    header.string_table_size = static_cast<uint32_t>(stringTableData.size());
    
    return offset;
}

Section& CofFile::getSection(size_t index) {
    if (index >= sections.size()) {
        throw std::out_of_range("Section index out of range");
    }
    
    return *sections[index];
}

Symbol& CofFile::getSymbol(size_t index) {
    if (index >= symbols.size()) {
        throw std::out_of_range("Symbol index out of range");
    }
    
    return *symbols[index];
}

Symbol* CofFile::getSymbolByName(const std::string& name) {
    for (const auto& symbol : symbols) {
        if (symbol->getName() == name) {
            return symbol.get();
        }
    }
    
    return nullptr;
}

void CofFile::setEntryPoint(uint64_t entryPoint) {
    header.entry_point = entryPoint;
}

size_t CofFile::getSectionCount() const {
    return sections.size();
}

size_t CofFile::getSymbolCount() const {
    return symbols.size();
}

size_t CofFile::getTargetCount() const {
    return targets.size();
}

void CofFile::updateOffsets() {
    // Calculate offsets
    uint32_t offset = header.header_size;
    
    // Target table
    header.target_table_offset = offset;
    offset += static_cast<uint32_t>(targets.size() * sizeof(TargetEntry));
    
    // Section table
    header.section_table_offset = offset;
    offset += static_cast<uint32_t>(sections.size() * sizeof(SectionEntry));
    
    // Symbol table
    header.symbol_table_offset = offset;
    offset += static_cast<uint32_t>(symbols.size() * sizeof(SymbolEntry));
    
    // String table
    header.string_table_offset = offset;
    offset += header.string_table_size;
    
    // Section data and relocations
    for (auto& section : sections) {
        // Align section data
        offset = (offset + section->getAlignment() - 1) & ~(section->getAlignment() - 1);
        
        // Update section offset and calculate relocation offset
        uint64_t sectionOffset = offset;
        offset += static_cast<uint32_t>(section->getSize());
        
        uint32_t relocOffset = offset;
        offset += static_cast<uint32_t>(section->getRelocations().size() * sizeof(RelocationEntry));
        
        // Update section entry
        uint32_t nameOffset = addString(section->getName());
        section->createEntry(nameOffset, sectionOffset, relocOffset);
    }
}

bool CofFile::write(const std::string& filename) {
    // Update offsets and finalize sections
    updateOffsets();
    
    // Open the output file
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        LOG_ERROR("Failed to open output file: " + filename);
        return false;
    }
    
    // Write the header
    outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Write the target table
    for (const auto& target : targets) {
        outFile.write(reinterpret_cast<const char*>(&target), sizeof(target));
    }
    
    // Write the section table
    for (const auto& section : sections) {
        SectionEntry entry = section->createEntry(
            addString(section->getName()),
            0, // Will be updated in final write
            0  // Will be updated in final write
        );
        outFile.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
    }
    
    // Write the symbol table
    for (const auto& symbol : symbols) {
        SymbolEntry entry = symbol->createEntry(addString(symbol->getName()));
        outFile.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
    }
    
    // Write the string table
    outFile.write(reinterpret_cast<const char*>(stringTableData.data()), stringTableData.size());
    
    // Write the section data and relocations
    for (const auto& section : sections) {
        // Align section data
        std::streampos pos = outFile.tellp();
        size_t padding = (section->getAlignment() - (pos % section->getAlignment())) % section->getAlignment();
        for (size_t i = 0; i < padding; i++) {
            outFile.put(0);
        }
        
        // Write section data
        const auto& data = section->getData();
        outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
        
        // Write relocations
        const auto& relocations = section->getRelocations();
        for (const auto& relocation : relocations) {
            outFile.write(reinterpret_cast<const char*>(&relocation), sizeof(relocation));
        }
    }
    
    // Check for errors
    if (!outFile) {
        LOG_ERROR("Error writing to output file: " + filename);
        return false;
    }
    
    return true;
}

std::unique_ptr<CofFile> CofFile::read(const std::string& filename) {
    // Open the input file
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile) {
        LOG_ERROR("Failed to open input file: " + filename);
        return nullptr;
    }
    
    // Create a new CofFile
    auto cof = std::make_unique<CofFile>();
    
    // Read the header
    inFile.read(reinterpret_cast<char*>(&cof->header), sizeof(cof->header));
    
    // Check the magic number
    if (cof->header.magic != COF_MAGIC) {
        LOG_ERROR("Invalid COF file format");
        return nullptr;
    }
    
    // Read the target table
    inFile.seekg(cof->header.target_table_offset);
    cof->targets.resize(cof->header.target_count);
    for (uint32_t i = 0; i < cof->header.target_count; i++) {
        inFile.read(reinterpret_cast<char*>(&cof->targets[i]), sizeof(TargetEntry));
    }
    
    // Read the section table
    inFile.seekg(cof->header.section_table_offset);
    std::vector<SectionEntry> sectionEntries(cof->header.section_count);
    for (uint32_t i = 0; i < cof->header.section_count; i++) {
        inFile.read(reinterpret_cast<char*>(&sectionEntries[i]), sizeof(SectionEntry));
    }
    
    // Read the symbol table
    inFile.seekg(cof->header.symbol_table_offset);
    std::vector<SymbolEntry> symbolEntries(cof->header.symbol_count);
    for (uint32_t i = 0; i < cof->header.symbol_count; i++) {
        inFile.read(reinterpret_cast<char*>(&symbolEntries[i]), sizeof(SymbolEntry));
    }
    
    // Read the string table
    inFile.seekg(cof->header.string_table_offset);
    cof->stringTableData.resize(cof->header.string_table_size);
    inFile.read(reinterpret_cast<char*>(cof->stringTableData.data()), cof->header.string_table_size);
    
    // Build the string table map
    for (uint32_t i = 0; i < cof->header.string_table_size; ) {
        const char* str = reinterpret_cast<const char*>(cof->stringTableData.data() + i);
        std::string strObj(str);
        cof->stringTable[strObj] = i;
        i += static_cast<uint32_t>(strObj.size() + 1); // Include null terminator
    }
    
    // Create sections
    for (uint32_t i = 0; i < cof->header.section_count; i++) {
        const SectionEntry& entry = sectionEntries[i];
        
        // Get the section name
        const char* namePtr = reinterpret_cast<const char*>(cof->stringTableData.data() + entry.name_offset);
        std::string name(namePtr);
        
        // Create the section
        auto section = std::make_unique<Section>(name, entry.type, entry.flags, entry.target_id, entry.alignment);
        section->setAddress(entry.address);
        
        // Read section data
        if (entry.size > 0) {
            std::vector<uint8_t> data(entry.size);
            inFile.seekg(entry.offset);
            inFile.read(reinterpret_cast<char*>(data.data()), entry.size);
            section->addData(data);
        }
        
        // Read relocations
        if (entry.relocation_count > 0) {
            inFile.seekg(entry.relocation_offset);
            for (uint32_t j = 0; j < entry.relocation_count; j++) {
                RelocationEntry reloc;
                inFile.read(reinterpret_cast<char*>(&reloc), sizeof(reloc));
                section->addRelocation(reloc.offset, reloc.symbol_index, reloc.type, reloc.addend, reloc.target_id);
            }
        }
        
        cof->sections.push_back(std::move(section));
    }
    
    // Create symbols
    for (uint32_t i = 0; i < cof->header.symbol_count; i++) {
        const SymbolEntry& entry = symbolEntries[i];
        
        // Get the symbol name
        const char* namePtr = reinterpret_cast<const char*>(cof->stringTableData.data() + entry.name_offset);
        std::string name(namePtr);
        
        // Create the symbol
        auto symbol = std::make_unique<Symbol>(name, entry.section_index, entry.value, entry.size, entry.type, entry.flags, entry.target_id);
        cof->symbols.push_back(std::move(symbol));
    }
    
    return cof;
}

} // namespace coil