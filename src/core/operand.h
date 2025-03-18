#ifndef COIL_CORE_OPERAND_H
#define COIL_CORE_OPERAND_H

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include "core/defs.h"

namespace coil {

/**
 * @brief Base class for all operand types
 */
class Operand {
public:
    virtual ~Operand() = default;
    
    /**
     * @brief Get the operand type byte
     * 
     * @return Type byte
     */
    virtual uint8_t getTypeByte() const = 0;
    
    /**
     * @brief Encode the operand to binary format
     * 
     * @return Binary encoding of the operand
     */
    virtual std::vector<uint8_t> encode() const = 0;
    
    /**
     * @brief Get string representation of the operand
     * 
     * @return String representation
     */
    virtual std::string toString() const = 0;
    
    /**
     * @brief Create a clone of this operand
     * 
     * @return Cloned operand
     */
    virtual std::unique_ptr<Operand> clone() const = 0;
    
    /**
     * @brief Decode an operand from binary data
     * 
     * @param data Binary data pointer
     * @param offset Offset into data (updated on return)
     * @return Decoded operand
     */
    static std::unique_ptr<Operand> decode(const uint8_t* data, size_t& offset);
};

/**
 * @brief Register operand
 */
class RegisterOperand : public Operand {
private:
    uint8_t regType;  // Register type
    uint8_t regId;    // Register ID
    uint8_t flags;    // Register flags

public:
    /**
     * @brief Construct a new Register Operand
     * 
     * @param type Register type
     * @param id Register ID
     * @param flg Register flags
     */
    RegisterOperand(uint8_t type, uint8_t id, uint8_t flg = 0);
    
    uint8_t getTypeByte() const override;
    std::vector<uint8_t> encode() const override;
    std::string toString() const override;
    std::unique_ptr<Operand> clone() const override;
    
    /**
     * @brief Get the register type
     * 
     * @return Register type
     */
    uint8_t getRegType() const;
    
    /**
     * @brief Get the register ID
     * 
     * @return Register ID
     */
    uint8_t getRegId() const;
    
    /**
     * @brief Get the register flags
     * 
     * @return Register flags
     */
    uint8_t getFlags() const;
    
    /**
     * @brief Decode a register operand from binary data
     * 
     * @param data Binary data pointer
     * @param offset Offset into data (updated on return)
     * @return Decoded register operand
     */
    static std::unique_ptr<RegisterOperand> decode(const uint8_t* data, size_t& offset);
};

/**
 * @brief Immediate operand
 */
class ImmediateOperand : public Operand {
private:
    uint8_t immType;             // Immediate type
    std::vector<uint8_t> value;  // Immediate value

public:
    /**
     * @brief Construct a new Immediate Operand
     * 
     * @param type Immediate type
     * @param val Immediate value
     */
    ImmediateOperand(uint8_t type, const std::vector<uint8_t>& val);
    
    /**
     * @brief Construct a new Immediate Operand with int32 value
     * 
     * @param val Integer value
     */
    explicit ImmediateOperand(int32_t val);
    
    /**
     * @brief Construct a new Immediate Operand with int64 value
     * 
     * @param val Integer value
     */
    explicit ImmediateOperand(int64_t val);
    
    /**
     * @brief Construct a new Immediate Operand with float value
     * 
     * @param val Float value
     */
    explicit ImmediateOperand(float val);
    
    /**
     * @brief Construct a new Immediate Operand with double value
     * 
     * @param val Double value
     */
    explicit ImmediateOperand(double val);
    
    /**
     * @brief Construct a new Immediate Operand with symbol reference
     * 
     * @param symbol Symbol name
     */
    explicit ImmediateOperand(const std::string& symbol);
    
    uint8_t getTypeByte() const override;
    std::vector<uint8_t> encode() const override;
    std::string toString() const override;
    std::unique_ptr<Operand> clone() const override;
    
    /**
     * @brief Get the immediate type
     * 
     * @return Immediate type
     */
    uint8_t getImmType() const;
    
    /**
     * @brief Get the immediate value
     * 
     * @return Immediate value bytes
     */
    const std::vector<uint8_t>& getValue() const;
    
    /**
     * @brief Get the immediate value as int32
     * 
     * @return int32 value
     */
    int32_t getInt32Value() const;
    
    /**
     * @brief Get the immediate value as int64
     * 
     * @return int64 value
     */
    int64_t getInt64Value() const;
    
    /**
     * @brief Get the immediate value as float
     * 
     * @return float value
     */
    float getFloatValue() const;
    
    /**
     * @brief Get the immediate value as double
     * 
     * @return double value
     */
    double getDoubleValue() const;
    
    /**
     * @brief Decode an immediate operand from binary data
     * 
     * @param data Binary data pointer
     * @param offset Offset into data (updated on return)
     * @return Decoded immediate operand
     */
    static std::unique_ptr<ImmediateOperand> decode(const uint8_t* data, size_t& offset);
};

/**
 * @brief Memory operand
 */
class MemoryOperand : public Operand {
private:
    uint8_t memType;          // Memory access type
    std::vector<uint8_t> data; // Memory operand data

public:
    /**
     * @brief Construct a new Memory Operand
     * 
     * @param type Memory access type
     * @param opData Memory operand data
     */
    MemoryOperand(uint8_t type, const std::vector<uint8_t>& opData);
    
    /**
     * @brief Construct a register indirect memory operand
     * 
     * @param regId Register ID
     */
    explicit MemoryOperand(uint8_t regId);
    
    /**
     * @brief Construct a register + displacement memory operand
     * 
     * @param regId Register ID
     * @param disp Displacement value
     */
    MemoryOperand(uint8_t regId, int32_t disp);
    
    /**
     * @brief Construct a register + register memory operand
     * 
     * @param regId1 Base register ID
     * @param regId2 Index register ID
     */
    MemoryOperand(uint8_t regId1, uint8_t regId2);
    
    /**
     * @brief Construct a register + scaled register memory operand
     * 
     * @param regId1 Base register ID
     * @param regId2 Index register ID
     * @param scale Scale factor
     */
    MemoryOperand(uint8_t regId1, uint8_t regId2, uint8_t scale);
    
    uint8_t getTypeByte() const override;
    std::vector<uint8_t> encode() const override;
    std::string toString() const override;
    std::unique_ptr<Operand> clone() const override;
    
    /**
     * @brief Get the memory access type
     * 
     * @return Memory access type
     */
    uint8_t getMemType() const;
    
    /**
     * @brief Get the memory operand data
     * 
     * @return Memory operand data bytes
     */
    const std::vector<uint8_t>& getData() const;
    
    /**
     * @brief Decode a memory operand from binary data
     * 
     * @param data Binary data pointer
     * @param offset Offset into data (updated on return)
     * @return Decoded memory operand
     */
    static std::unique_ptr<MemoryOperand> decode(const uint8_t* data, size_t& offset);
};

/**
 * @brief Variable operand
 */
class VariableOperand : public Operand {
private:
    uint8_t varType;  // Variable reference type
    uint8_t varId;    // Variable ID

public:
    /**
     * @brief Construct a new Variable Operand
     * 
     * @param type Variable reference type
     * @param id Variable ID
     */
    VariableOperand(uint8_t type, uint8_t id);
    
    /**
     * @brief Construct a direct variable reference
     * 
     * @param id Variable ID
     */
    explicit VariableOperand(uint8_t id);
    
    uint8_t getTypeByte() const override;
    std::vector<uint8_t> encode() const override;
    std::string toString() const override;
    std::unique_ptr<Operand> clone() const override;
    
    /**
     * @brief Get the variable reference type
     * 
     * @return Variable reference type
     */
    uint8_t getVarType() const;
    
    /**
     * @brief Get the variable ID
     * 
     * @return Variable ID
     */
    uint8_t getVarId() const;
    
    /**
     * @brief Decode a variable operand from binary data
     * 
     * @param data Binary data pointer
     * @param offset Offset into data (updated on return)
     * @return Decoded variable operand
     */
    static std::unique_ptr<VariableOperand> decode(const uint8_t* data, size_t& offset);
};

} // namespace coil

#endif // COIL_CORE_OPERAND_H