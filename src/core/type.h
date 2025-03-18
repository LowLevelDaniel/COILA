#ifndef COIL_CORE_TYPE_H
#define COIL_CORE_TYPE_H

#include <cstdint>
#include <string>
#include "core/defs.h"

namespace coil {

/**
 * @brief Type descriptor
 * 
 * Describes a data type in the COIL language.
 */
class Type {
private:
    uint16_t typeId;     // Type ID
    uint16_t flags;      // Type flags
    uint32_t size;       // Type size in bytes
    uint32_t alignment;  // Type alignment in bytes
    std::string name;    // Type name

public:
    /**
     * @brief Construct a new Type
     * 
     * @param tId Type ID
     * @param tSize Type size
     * @param tAlign Type alignment
     * @param tFlags Type flags
     * @param tName Type name
     */
    Type(uint16_t tId, uint32_t tSize, uint32_t tAlign, uint16_t tFlags = 0, 
         const std::string& tName = "");
    
    /**
     * @brief Get type ID
     * 
     * @return Type ID
     */
    uint16_t getTypeId() const;
    
    /**
     * @brief Get type flags
     * 
     * @return Type flags
     */
    uint16_t getFlags() const;
    
    /**
     * @brief Get type size
     * 
     * @return Type size in bytes
     */
    uint32_t getSize() const;
    
    /**
     * @brief Get type alignment
     * 
     * @return Type alignment in bytes
     */
    uint32_t getAlignment() const;
    
    /**
     * @brief Get type name
     * 
     * @return Type name
     */
    const std::string& getName() const;
    
    /**
     * @brief Set type name
     * 
     * @param tName Type name
     */
    void setName(const std::string& tName);
    
    /**
     * @brief Check if the type is a pointer
     * 
     * @return true if pointer, false otherwise
     */
    bool isPointer() const;
    
    /**
     * @brief Check if the type is a vector
     * 
     * @return true if vector, false otherwise
     */
    bool isVector() const;
    
    /**
     * @brief Get the base type for pointer or vector types
     * 
     * @return Base type ID
     */
    uint16_t getBaseType() const;
    
    /**
     * @brief Get string representation of the type
     * 
     * @return Type string
     */
    std::string toString() const;
    
    /**
     * @brief Create a type from a basic type ID
     * 
     * @param typeId Basic type ID
     * @return Type object
     */
    static Type fromBasicType(uint16_t typeId);
};

} // namespace coil

#endif // COIL_CORE_TYPE_H