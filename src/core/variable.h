#ifndef COIL_CORE_VARIABLE_H
#define COIL_CORE_VARIABLE_H

#include <cstdint>
#include <string>
#include <vector>
#include "core/defs.h"
#include "core/type.h"

namespace coil {

/**
 * @brief Variable descriptor
 * 
 * Describes a variable in the COIL language.
 */
class Variable {
private:
    uint8_t varId;       // Variable ID
    uint8_t storageClass; // Storage class
    uint16_t typeId;     // Type ID
    std::string name;    // Variable name
    std::vector<uint8_t> initialValue; // Initial value

public:
    /**
     * @brief Construct a new Variable
     * 
     * @param vId Variable ID
     * @param vType Type ID
     * @param vStorage Storage class
     * @param vName Variable name
     */
    Variable(uint8_t vId, uint16_t vType, uint8_t vStorage = STORAGE_AUTO, 
             const std::string& vName = "");
    
    /**
     * @brief Get variable ID
     * 
     * @return Variable ID
     */
    uint8_t getVarId() const;
    
    /**
     * @brief Get storage class
     * 
     * @return Storage class
     */
    uint8_t getStorageClass() const;
    
    /**
     * @brief Get type ID
     * 
     * @return Type ID
     */
    uint16_t getTypeId() const;
    
    /**
     * @brief Get variable name
     * 
     * @return Variable name
     */
    const std::string& getName() const;
    
    /**
     * @brief Set variable name
     * 
     * @param vName Variable name
     */
    void setName(const std::string& vName);
    
    /**
     * @brief Set initial value
     * 
     * @param value Initial value bytes
     */
    void setInitialValue(const std::vector<uint8_t>& value);
    
    /**
     * @brief Get initial value
     * 
     * @return Initial value bytes
     */
    const std::vector<uint8_t>& getInitialValue() const;
    
    /**
     * @brief Check if the variable has an initial value
     * 
     * @return true if has initial value, false otherwise
     */
    bool hasInitialValue() const;
    
    /**
     * @brief Get string representation of the variable
     * 
     * @return Variable string
     */
    std::string toString() const;
    
    /**
     * @brief Get string representation of the variable ID
     * 
     * @return Variable ID string
     */
    std::string getIdString() const;
};

} // namespace coil

#endif // COIL_CORE_VARIABLE_H