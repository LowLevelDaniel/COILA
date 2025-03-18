#ifndef COIL_CORE_INSTRUCTION_H
#define COIL_CORE_INSTRUCTION_H

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include "core/defs.h"
#include "core/operand.h"

namespace coil {

// Forward declaration
class Operand;
using OperandPtr = std::unique_ptr<Operand>;

/**
 * @brief Instruction representation
 * 
 * Represents a COIL instruction with its category, operation,
 * operands, and extended data.
 */
class Instruction {
private:
    uint8_t category;  // Instruction category (bits 7-5 of opcode)
    uint8_t operation; // Operation within category (bits 4-0 of opcode)
    std::vector<OperandPtr> operands; // Instruction operands
    std::vector<uint8_t> extendedData; // Extended data

public:
    /**
     * @brief Construct a new Instruction
     * 
     * @param cat Instruction category
     * @param op Operation within category
     */
    Instruction(uint8_t cat, uint8_t op);
    
    /**
     * @brief Add an operand to the instruction
     * 
     * @param op Operand to add
     */
    void addOperand(OperandPtr op);
    
    /**
     * @brief Set extended data for the instruction
     * 
     * @param data Extended data
     */
    void setExtendedData(const std::vector<uint8_t>& data);
    
    /**
     * @brief Get the instruction category
     * 
     * @return Instruction category
     */
    uint8_t getCategory() const;
    
    /**
     * @brief Get the operation within category
     * 
     * @return Operation code
     */
    uint8_t getOperation() const;
    
    /**
     * @brief Get the opcode (combined category and operation)
     * 
     * @return Combined opcode byte
     */
    uint8_t getOpcode() const;
    
    /**
     * @brief Get the operands
     * 
     * @return Vector of operands
     */
    const std::vector<OperandPtr>& getOperands() const;
    
    /**
     * @brief Get the extended data
     * 
     * @return Extended data bytes
     */
    const std::vector<uint8_t>& getExtendedData() const;
    
    /**
     * @brief Encode the instruction to binary format
     * 
     * @return Binary encoding of the instruction
     */
    std::vector<uint8_t> encode() const;
    
    /**
     * @brief Decode an instruction from binary data
     * 
     * @param data Binary data pointer
     * @param offset Offset into data (updated on return)
     * @return Decoded instruction
     */
    static std::unique_ptr<Instruction> decode(const uint8_t* data, size_t& offset);
    
    /**
     * @brief Get string representation of the instruction
     * 
     * @return String representation
     */
    std::string toString() const;

    /**
    * @brief Create a clone of this instruction
    * 
    * @return Cloned instruction
    */
    std::unique_ptr<Instruction> clone() const;
};

} // namespace coil

#endif // COIL_CORE_INSTRUCTION_H