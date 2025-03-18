#ifndef COIL_TARGET_TARGET_H
#define COIL_TARGET_TARGET_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include "core/defs.h"
#include "core/instruction.h"
#include "core/register.h"

namespace coil {

class Instruction;
class Function;

/**
 * @brief Register mapping
 */
struct RegisterMapping {
    uint8_t vregId;         // Virtual register ID
    uint8_t pregId;         // Physical register ID
    uint8_t pregClass;      // Physical register class
    uint8_t flags;          // Mapping flags
    
    RegisterMapping(uint8_t vId, uint8_t pId, uint8_t pClass, uint8_t flg = 0)
        : vregId(vId), pregId(pId), pregClass(pClass), flags(flg) {}
};

/**
 * @brief Target architecture base class
 * 
 * Defines the interface for target architectures.
 */
class Target {
protected:
    uint32_t id;                    // Target ID
    uint8_t archClass;              // Architecture class
    uint8_t archType;               // Architecture type
    uint8_t wordSize;               // Word size in bits
    uint8_t endianness;             // Byte ordering
    uint32_t features;              // Feature flags
    uint32_t extensions;            // Extension flags
    std::vector<RegisterMapping> regMappings; // Register mappings
    uint32_t defaultAbiId;          // Default ABI ID
    std::string name;               // Target name
    
public:
    /**
     * @brief Construct a new Target
     * 
     * @param targetId Target ID
     * @param aClass Architecture class
     * @param aType Architecture type
     * @param wSize Word size in bits
     * @param end Endianness
     * @param targetName Target name
     */
    Target(uint32_t targetId, uint8_t aClass, uint8_t aType, uint8_t wSize, uint8_t end, const std::string& targetName);
    
    /**
     * @brief Destroy the Target
     */
    virtual ~Target() = default;
    
    /**
     * @brief Get the target ID
     * 
     * @return Target ID
     */
    uint32_t getId() const;
    
    /**
     * @brief Get the architecture class
     * 
     * @return Architecture class
     */
    uint8_t getArchClass() const;
    
    /**
     * @brief Get the architecture type
     * 
     * @return Architecture type
     */
    uint8_t getArchType() const;
    
    /**
     * @brief Get the word size
     * 
     * @return Word size in bits
     */
    uint8_t getWordSize() const;
    
    /**
     * @brief Get the endianness
     * 
     * @return Endianness
     */
    uint8_t getEndianness() const;
    
    /**
     * @brief Get the feature flags
     * 
     * @return Feature flags
     */
    uint32_t getFeatures() const;
    
    /**
     * @brief Set the feature flags
     * 
     * @param feat Feature flags
     */
    void setFeatures(uint32_t feat);
    
    /**
     * @brief Get the extension flags
     * 
     * @return Extension flags
     */
    uint32_t getExtensions() const;
    
    /**
     * @brief Set the extension flags
     * 
     * @param ext Extension flags
     */
    void setExtensions(uint32_t ext);
    
    /**
     * @brief Add a register mapping
     * 
     * @param mapping Register mapping
     */
    void addRegisterMapping(const RegisterMapping& mapping);
    
    /**
     * @brief Get the register mappings
     * 
     * @return Vector of register mappings
     */
    const std::vector<RegisterMapping>& getRegisterMappings() const;
    
    /**
     * @brief Set the default ABI ID
     * 
     * @param abiId Default ABI ID
     */
    void setDefaultAbiId(uint32_t abiId);
    
    /**
     * @brief Get the default ABI ID
     * 
     * @return Default ABI ID
     */
    uint32_t getDefaultAbiId() const;
    
    /**
     * @brief Get the target name
     * 
     * @return Target name
     */
    const std::string& getName() const;
    
    /**
     * @brief Transform an instruction for this target
     * 
     * @param inst Instruction to transform
     */
    virtual void transformInstruction(Instruction& inst) = 0;
    
    /**
     * @brief Allocate registers for a function
     * 
     * @param func Function to allocate registers for
     */
    virtual void allocateRegisters(Function& func) = 0;
    
    /**
     * @brief Generate function prologue
     * 
     * @param func Function to generate prologue for
     * @return Generated instructions
     */
    virtual std::vector<std::unique_ptr<Instruction>> generatePrologue(Function& func) = 0;
    
    /**
     * @brief Generate function epilogue
     * 
     * @param func Function to generate epilogue for
     * @return Generated instructions
     */
    virtual std::vector<std::unique_ptr<Instruction>> generateEpilogue(Function& func) = 0;
    
    /**
     * @brief Get the physical register ID for a virtual register
     * 
     * @param vregId Virtual register ID
     * @return Physical register ID, or 0xFF if not found
     */
    uint8_t getPhysicalRegister(uint8_t vregId) const;
    
    /**
     * @brief Create a target from configuration data
     * 
     * @param targetId Target ID
     * @param configData Configuration data
     * @return Target object
     */
    static std::unique_ptr<Target> createFromConfig(uint32_t targetId, const std::vector<uint8_t>& configData);
    
    /**
     * @brief Create a target from architecture type
     * 
     * @param targetId Target ID
     * @param archType Architecture type
     * @return Target object
     */
    static std::unique_ptr<Target> createFromArchType(uint32_t targetId, uint8_t archType);
};

} // namespace coil

#endif // COIL_TARGET_TARGET_H