#ifndef COIL_CORE_REGISTER_H
#define COIL_CORE_REGISTER_H

#include <cstdint>
#include <string>
#include "core/defs.h"

namespace coil {

/**
 * @brief Register descriptor
 * 
 * Describes a physical or virtual register.
 */
class Register {
private:
    uint8_t regClass;  // Register class (GP, FP, VEC, ...)
    uint8_t regId;     // Register ID
    uint8_t regSize;   // Register size in bytes
    uint8_t flags;     // Register flags
    std::string name;  // Register name

public:
    /**
     * @brief Construct a new Register
     * 
     * @param rClass Register class
     * @param rId Register ID
     * @param rSize Register size
     * @param rFlags Register flags
     * @param rName Register name
     */
    Register(uint8_t rClass, uint8_t rId, uint8_t rSize, uint8_t rFlags = 0, 
             const std::string& rName = "");
    
    /**
     * @brief Get register class
     * 
     * @return Register class
     */
    uint8_t getRegClass() const;
    
    /**
     * @brief Get register ID
     * 
     * @return Register ID
     */
    uint8_t getRegId() const;
    
    /**
     * @brief Get register size
     * 
     * @return Register size in bytes
     */
    uint8_t getRegSize() const;
    
    /**
     * @brief Get register flags
     * 
     * @return Register flags
     */
    uint8_t getFlags() const;
    
    /**
     * @brief Get register name
     * 
     * @return Register name
     */
    const std::string& getName() const;
    
    /**
     * @brief Set register name
     * 
     * @param rName Register name
     */
    void setName(const std::string& rName);
    
    /**
     * @brief Get register ID as string
     * 
     * @return Register ID string
     */
    std::string getIdString() const;
};

} // namespace coil

#endif // COIL_CORE_REGISTER_H