#ifndef COIL_BINARY_SECTION_H
#define COIL_BINARY_SECTION_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "core/defs.h"
#include "core/instruction.h"

namespace coil {

/**
 * @brief Section table entry in COF
 */
struct SectionEntry {
    uint32_t name_offset;        // Offset to section name in string table
    uint32_t type;               // Section type
    uint32_t flags;              // Section flags
    uint32_t target_id;          // Target architecture (0 for generic)
    uint64_t address;            // Virtual address (if applicable)
    uint64_t size;               // Section size
    uint64_t offset;             // File offset to section data
    uint32_t alignment;          // Section alignment
    uint32_t relocation_count;   // Number of relocations
    uint32_t relocation_offset;  // Offset to relocation table
};

/**
 * @brief Relocation entry
 */
struct RelocationEntry {
    uint64_t offset;             // Offset within section
    uint32_t symbol_index;       // Symbol table index
    uint32_t type;               // Relocation type
    int64_t  addend;             // Addend value
    uint32_t target_id;          // Target architecture
};

/**
 * @brief Section class
 * 
 * Represents a section in a COIL Object Format file.
 */
class Section {
private:
    std::string name;            // Section name
    uint32_t type;               // Section type
    uint32_t flags;              // Section flags
    uint32_t targetId;           // Target architecture
    uint64_t address;            // Virtual address
    uint32_t alignment;          // Section alignment
    std::vector<uint8_t> data;   // Section data
    std::vector<RelocationEntry> relocations; // Relocations

    // For code sections
    std::vector<std::unique_ptr<Instruction>> instructions; // Instructions

public:
    /**
     * @brief Construct a new Section
     * 
     * @param name Section name
     * @param type Section type
     * @param flags Section flags
     * @param targetId Target architecture (0 for generic)
     * @param alignment Section alignment
     */
    Section(const std::string& name, uint32_t type, uint32_t flags, uint32_t targetId = 0, uint32_t alignment = 4);
    
    /**
     * @brief Get the section name
     * 
     * @return Section name
     */
    const std::string& getName() const;
    
    /**
     * @brief Get the section type
     * 
     * @return Section type
     */
    uint32_t getType() const;
    
    /**
     * @brief Get the section flags
     * 
     * @return Section flags
     */
    uint32_t getFlags() const;
    
    /**
     * @brief Get the target architecture
     * 
     * @return Target architecture ID
     */
    uint32_t getTargetId() const;
    
    /**
     * @brief Get the virtual address
     * 
     * @return Virtual address
     */
    uint64_t getAddress() const;
    
    /**
     * @brief Set the virtual address
     * 
     * @param addr Virtual address
     */
    void setAddress(uint64_t addr);
    
    /**
     * @brief Get the section alignment
     * 
     * @return Section alignment
     */
    uint32_t getAlignment() const;
    
    /**
     * @brief Get the section data
     * 
     * @return Section data bytes
     */
    const std::vector<uint8_t>& getData() const;
    
    /**
     * @brief Get the section data size
     * 
     * @return Section data size
     */
    size_t getSize() const;
    
    /**
     * @brief Add data to the section
     * 
     * @param newData Data to add
     * @return Offset to the added data
     */
    uint64_t addData(const std::vector<uint8_t>& newData);
    
    /**
     * @brief Add a relocation
     * 
     * @param offset Offset within section
     * @param symbolIndex Symbol table index
     * @param type Relocation type
     * @param addend Addend value
     * @param targetId Target architecture
     */
    void addRelocation(uint64_t offset, uint32_t symbolIndex, uint32_t type, int64_t addend, uint32_t targetId);
    
    /**
     * @brief Get the relocations
     * 
     * @return Vector of relocations
     */
    const std::vector<RelocationEntry>& getRelocations() const;
    
    /**
     * @brief Add an instruction to a code section
     * 
     * @param instruction Instruction to add
     * @return Offset to the added instruction
     */
    uint64_t addInstruction(std::unique_ptr<Instruction> instruction);
    
    /**
     * @brief Get the instructions in a code section
     * 
     * @return Vector of instructions
     */
    const std::vector<std::unique_ptr<Instruction>>& getInstructions() const;
    
    /**
     * @brief Get the specified bytes from section data
     * 
     * @param offset Offset within the section
     * @param size Number of bytes to read
     * @return Vector of bytes
     */
    std::vector<uint8_t> getBytes(uint64_t offset, size_t size) const;
    
    /**
     * @brief Fill section data to be 0
     * 
     * @param size Size to fill
     * @return Offset to the filled data
     */
    uint64_t fillZero(size_t size);
    
    /**
     * @brief Align the current position to the specified boundary
     * 
     * @param alignment Alignment boundary
     * @return Aligned position
     */
    uint64_t align(uint32_t alignment);
    
    /**
     * @brief Finalize the section, converting instructions to binary data
     */
    void finalize();
    
    /**
     * @brief Create a SectionEntry for this section
     * 
     * @param nameOffset Offset to section name in string table
     * @param sectionOffset Offset to section data in file
     * @param relocOffset Offset to relocation table in file
     * @return SectionEntry object
     */
    SectionEntry createEntry(uint32_t nameOffset, uint64_t sectionOffset, uint32_t relocOffset) const;
};

} // namespace coil

#endif // COIL_BINARY_SECTION_H