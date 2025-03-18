#ifndef COIL_BINARY_COF_H
#define COIL_BINARY_COF_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include "core/defs.h"
#include "binary/section.h"
#include "binary/symbol.h"

namespace coil {

// Magic number for COF files ("COIL")
constexpr uint32_t COF_MAGIC = 0x434F494C;

// Current COF format version
constexpr uint16_t COF_VERSION_MAJOR = 1;
constexpr uint16_t COF_VERSION_MINOR = 0;

/**
 * @brief COIL Object Format header
 */
struct CofHeader {
    uint32_t magic;              // Magic number ('COIL')
    uint16_t version_major;      // Format version major
    uint16_t version_minor;      // Format version minor
    uint32_t flags;              // Format flags
    uint32_t target_count;       // Number of target architectures
    uint32_t section_count;      // Number of sections
    uint32_t symbol_count;       // Number of symbols
    uint32_t string_table_size;  // Size of string table
    uint64_t entry_point;        // Entry point (if executable)
    uint64_t timestamp;          // Creation timestamp
    uint8_t  uuid[16];           // Unique identifier
    uint32_t header_size;        // Size of the header
    uint32_t section_table_offset; // Offset to section table
    uint32_t symbol_table_offset;  // Offset to symbol table
    uint32_t string_table_offset;  // Offset to string table
    uint32_t target_table_offset;  // Offset to target architecture table
};

/**
 * @brief Target architecture entry
 */
struct TargetEntry {
    uint32_t target_id;          // Target identifier
    uint32_t arch_type;          // Architecture type
    uint32_t features;           // Feature flags
    uint32_t name_offset;        // Offset to target name in string table
    uint32_t config_offset;      // Offset to configuration data
    uint32_t config_size;        // Size of configuration data
};

/**
 * @brief COIL Object Format manager
 * 
 * Manages the creation and parsing of COIL Object Format files.
 */
class CofFile {
private:
    CofHeader header;
    std::vector<TargetEntry> targets;
    std::vector<std::unique_ptr<Section>> sections;
    std::vector<std::unique_ptr<Symbol>> symbols;
    std::map<std::string, uint32_t> stringTable;
    std::vector<uint8_t> stringTableData;
    
    // Utility methods
    void updateOffsets();

public:
    /**
     * @brief Construct a new CofFile
     */
    CofFile();
    
    /**
     * @brief Add a target architecture
     * 
     * @param archType Architecture type
     * @param features Feature flags
     * @param name Target name
     * @return Target ID
     */
    uint32_t addTarget(uint32_t archType, uint32_t features, const std::string& name);
    
    /**
     * @brief Add a section
     * 
     * @param name Section name
     * @param type Section type
     * @param flags Section flags
     * @param targetId Target architecture (0 for generic)
     * @return Reference to the new section
     */
    Section& addSection(const std::string& name, uint32_t type, uint32_t flags, uint32_t targetId = 0);
    
    /**
     * @brief Add a symbol
     * 
     * @param name Symbol name
     * @param sectionIndex Section index (0 for external)
     * @param value Symbol value (address or offset)
     * @param size Symbol size
     * @param type Symbol type
     * @param flags Symbol flags
     * @param targetId Target architecture (0 for generic)
     * @return Symbol ID
     */
    uint32_t addSymbol(const std::string& name, uint32_t sectionIndex, uint64_t value, 
                     uint64_t size, uint16_t type, uint16_t flags, uint32_t targetId = 0);
    
    /**
     * @brief Add a string to the string table
     * 
     * @param str String to add
     * @return Offset to the string in the string table
     */
    uint32_t addString(const std::string& str);
    
    /**
     * @brief Get a section by index
     * 
     * @param index Section index
     * @return Reference to the section
     */
    Section& getSection(size_t index);
    
    /**
     * @brief Get a symbol by index
     * 
     * @param index Symbol index
     * @return Reference to the symbol
     */
    Symbol& getSymbol(size_t index);
    
    /**
     * @brief Get a symbol by name
     * 
     * @param name Symbol name
     * @return Reference to the symbol, or nullptr if not found
     */
    Symbol* getSymbolByName(const std::string& name);
    
    /**
     * @brief Set the entry point
     * 
     * @param entryPoint Entry point address
     */
    void setEntryPoint(uint64_t entryPoint);
    
    /**
     * @brief Get number of sections
     * 
     * @return Number of sections
     */
    size_t getSectionCount() const;
    
    /**
     * @brief Get number of symbols
     * 
     * @return Number of symbols
     */
    size_t getSymbolCount() const;
    
    /**
     * @brief Get number of targets
     * 
     * @return Number of targets
     */
    size_t getTargetCount() const;
    
    /**
     * @brief Write the COF file to disk
     * 
     * @param filename Output filename
     * @return true if successful, false otherwise
     */
    bool write(const std::string& filename);
    
    /**
     * @brief Read a COF file from disk
     * 
     * @param filename Input filename
     * @return CofFile object, or nullptr if failed
     */
    static std::unique_ptr<CofFile> read(const std::string& filename);
};

} // namespace coil

#endif // COIL_BINARY_COF_H