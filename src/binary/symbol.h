#ifndef COIL_BINARY_SYMBOL_H
#define COIL_BINARY_SYMBOL_H

#include <cstdint>
#include <string>
#include "core/defs.h"

namespace coil {

/**
 * @brief Symbol table entry in COF
 */
struct SymbolEntry {
    uint32_t name_offset;        // Offset to symbol name in string table
    uint32_t section_index;      // Section index (0 for external)
    uint64_t value;              // Symbol value (address or offset)
    uint64_t size;               // Symbol size
    uint16_t type;               // Symbol type
    uint16_t flags;              // Symbol flags
    uint32_t target_id;          // Target architecture (0 for generic)
};

/**
 * @brief Symbol class
 * 
 * Represents a symbol in a COIL Object Format file.
 */
class Symbol {
private:
    std::string name;        // Symbol name
    uint32_t sectionIndex;   // Section index (0 for external)
    uint64_t value;          // Symbol value (address or offset)
    uint64_t size;           // Symbol size
    uint16_t type;           // Symbol type
    uint16_t flags;          // Symbol flags
    uint32_t targetId;       // Target architecture

public:
    /**
     * @brief Construct a new Symbol
     * 
     * @param name Symbol name
     * @param sectionIndex Section index (0 for external)
     * @param value Symbol value (address or offset)
     * @param size Symbol size
     * @param type Symbol type
     * @param flags Symbol flags
     * @param targetId Target architecture (0 for generic)
     */
    Symbol(const std::string& name, uint32_t sectionIndex, uint64_t value, 
           uint64_t size, uint16_t type, uint16_t flags, uint32_t targetId = 0);
    
    /**
     * @brief Get the symbol name
     * 
     * @return Symbol name
     */
    const std::string& getName() const;
    
    /**
     * @brief Get the section index
     * 
     * @return Section index
     */
    uint32_t getSectionIndex() const;
    
    /**
     * @brief Get the symbol value
     * 
     * @return Symbol value
     */
    uint64_t getValue() const;
    
    /**
     * @brief Set the symbol value
     * 
     * @param val New value
     */
    void setValue(uint64_t val);
    
    /**
     * @brief Get the symbol size
     * 
     * @return Symbol size
     */
    uint64_t getSize() const;
    
    /**
     * @brief Set the symbol size
     * 
     * @param s New size
     */
    void setSize(uint64_t s);
    
    /**
     * @brief Get the symbol type
     * 
     * @return Symbol type
     */
    uint16_t getType() const;
    
    /**
     * @brief Get the symbol flags
     * 
     * @return Symbol flags
     */
    uint16_t getFlags() const;
    
    /**
     * @brief Check if the symbol has a specific flag
     * 
     * @param flag Flag to check
     * @return true if flag is set, false otherwise
     */
    bool hasFlag(uint16_t flag) const;
    
    /**
     * @brief Add a flag to the symbol
     * 
     * @param flag Flag to add
     */
    void addFlag(uint16_t flag);
    
    /**
     * @brief Get the target architecture
     * 
     * @return Target architecture ID
     */
    uint32_t getTargetId() const;
    
    /**
     * @brief Check if the symbol is global
     * 
     * @return true if global, false otherwise
     */
    bool isGlobal() const;
    
    /**
     * @brief Check if the symbol is undefined
     * 
     * @return true if undefined, false otherwise
     */
    bool isUndefined() const;
    
    /**
     * @brief Check if the symbol is a function
     * 
     * @return true if function, false otherwise
     */
    bool isFunction() const;
    
    /**
     * @brief Create a SymbolEntry for this symbol
     * 
     * @param nameOffset Offset to symbol name in string table
     * @return SymbolEntry object
     */
    SymbolEntry createEntry(uint32_t nameOffset) const;
};

} // namespace coil

#endif // COIL_BINARY_SYMBOL_H