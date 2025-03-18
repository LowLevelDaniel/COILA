#include "core/operand.h"
#include <sstream>
#include <iomanip>
#include <cstring>

namespace coil {

std::unique_ptr<Operand> Operand::decode(const uint8_t* data, size_t& offset) {
    if (data == nullptr) {
        return nullptr;
    }
    
    uint8_t typeByte = data[offset];
    uint8_t operandClass = typeByte & 0xC0; // Upper 2 bits
    
    switch (operandClass) {
        case OPERAND_REGISTER:
            return RegisterOperand::decode(data, offset);
        case OPERAND_IMMEDIATE:
            return ImmediateOperand::decode(data, offset);
        case OPERAND_MEMORY:
            return MemoryOperand::decode(data, offset);
        case OPERAND_VARIABLE:
            return VariableOperand::decode(data, offset);
        default:
            // Unknown operand class
            return nullptr;
    }
}

//
// RegisterOperand implementation
//

RegisterOperand::RegisterOperand(uint8_t type, uint8_t id, uint8_t flg)
    : regType(type), regId(id), flags(flg) {
}

uint8_t RegisterOperand::getTypeByte() const {
    return OPERAND_REGISTER | regType;
}

std::vector<uint8_t> RegisterOperand::encode() const {
    std::vector<uint8_t> result;
    result.push_back(getTypeByte());
    result.push_back(regId);
    result.push_back(flags);
    return result;
}

std::string RegisterOperand::toString() const {
    std::ostringstream oss;
    
    // Add register prefix based on type
    switch (regType) {
        case REG_GP:
            oss << "R" << static_cast<int>(regId);
            break;
        case REG_FP:
            oss << "F" << static_cast<int>(regId);
            break;
        case REG_VEC:
            oss << "V" << static_cast<int>(regId);
            break;
        case REG_SPECIAL:
            // Special registers have specific names
            switch (regId) {
                case REG_PC:    oss << "PC"; break;
                case REG_SP:    oss << "SP"; break;
                case REG_FP:    oss << "FP"; break;
                case REG_FLAGS: oss << "FLAGS"; break;
                case REG_LR:    oss << "LR"; break;
                default:        oss << "SPECIAL" << static_cast<int>(regId); break;
            }
            break;
        default:
            oss << "REG" << static_cast<int>(regId);
            break;
    }
    
    // Add flags if present
    if (flags != 0) {
        oss << "." << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(flags);
    }
    
    return oss.str();
}

std::unique_ptr<Operand> RegisterOperand::clone() const {
    return std::make_unique<RegisterOperand>(regType, regId, flags);
}

uint8_t RegisterOperand::getRegType() const {
    return regType;
}

uint8_t RegisterOperand::getRegId() const {
    return regId;
}

uint8_t RegisterOperand::getFlags() const {
    return flags;
}

std::unique_ptr<RegisterOperand> RegisterOperand::decode(const uint8_t* data, size_t& offset) {
    if (data == nullptr) {
        return nullptr;
    }
    
    uint8_t typeByte = data[offset++];
    uint8_t regType = typeByte & 0x3F; // Lower 6 bits
    uint8_t regId = data[offset++];
    uint8_t flags = data[offset++];
    
    return std::make_unique<RegisterOperand>(regType, regId, flags);
}

//
// ImmediateOperand implementation
//

ImmediateOperand::ImmediateOperand(uint8_t type, const std::vector<uint8_t>& val)
    : immType(type), value(val) {
}

ImmediateOperand::ImmediateOperand(int32_t val) 
    : immType(IMM_INT32), value(4) {
    // Store as little-endian bytes
    value[0] = val & 0xFF;
    value[1] = (val >> 8) & 0xFF;
    value[2] = (val >> 16) & 0xFF;
    value[3] = (val >> 24) & 0xFF;
}

ImmediateOperand::ImmediateOperand(int64_t val)
    : immType(IMM_INT64), value(8) {
    // Store as little-endian bytes
    value[0] = val & 0xFF;
    value[1] = (val >> 8) & 0xFF;
    value[2] = (val >> 16) & 0xFF;
    value[3] = (val >> 24) & 0xFF;
    value[4] = (val >> 32) & 0xFF;
    value[5] = (val >> 40) & 0xFF;
    value[6] = (val >> 48) & 0xFF;
    value[7] = (val >> 56) & 0xFF;
}

ImmediateOperand::ImmediateOperand(float val)
    : immType(IMM_FLOAT32), value(4) {
    // Copy raw bytes
    std::memcpy(value.data(), &val, 4);
}

ImmediateOperand::ImmediateOperand(double val)
    : immType(IMM_FLOAT64), value(8) {
    // Copy raw bytes
    std::memcpy(value.data(), &val, 8);
}

ImmediateOperand::ImmediateOperand(const std::string& symbol)
    : immType(IMM_SYMBOL), value(symbol.begin(), symbol.end()) {
    // Add null terminator
    value.push_back(0);
}

uint8_t ImmediateOperand::getTypeByte() const {
    return OPERAND_IMMEDIATE | immType;
}

std::vector<uint8_t> ImmediateOperand::encode() const {
    std::vector<uint8_t> result;
    result.push_back(getTypeByte());
    result.insert(result.end(), value.begin(), value.end());
    return result;
}

std::string ImmediateOperand::toString() const {
    std::ostringstream oss;
    
    switch (immType) {
        case IMM_INT8:
        case IMM_INT16:
        case IMM_INT32:
        case IMM_INT64:
            // Output as decimal
            oss << getInt64Value();
            break;
        case IMM_FLOAT32:
            // Output float with precision
            oss << std::fixed << std::setprecision(6) << getFloatValue();
            break;
        case IMM_FLOAT64:
            // Output double with precision
            oss << std::fixed << std::setprecision(15) << getDoubleValue();
            break;
        case IMM_SYMBOL:
            // Output symbol name
            oss << std::string(reinterpret_cast<const char*>(value.data()), value.size() - 1); // Exclude null terminator
            break;
        default:
            // Output raw bytes
            oss << "0x";
            for (uint8_t byte : value) {
                oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
            }
            break;
    }
    
    return oss.str();
}

std::unique_ptr<Operand> ImmediateOperand::clone() const {
    return std::make_unique<ImmediateOperand>(immType, value);
}

uint8_t ImmediateOperand::getImmType() const {
    return immType;
}

const std::vector<uint8_t>& ImmediateOperand::getValue() const {
    return value;
}

int32_t ImmediateOperand::getInt32Value() const {
    if (value.size() < 4) {
        return 0;
    }
    
    int32_t result = 0;
    result |= static_cast<int32_t>(value[0]);
    result |= static_cast<int32_t>(value[1]) << 8;
    result |= static_cast<int32_t>(value[2]) << 16;
    result |= static_cast<int32_t>(value[3]) << 24;
    return result;
}

int64_t ImmediateOperand::getInt64Value() const {
    if (value.size() < 8) {
        // Convert smaller values to int64_t
        int64_t result = 0;
        for (size_t i = 0; i < value.size(); i++) {
            result |= static_cast<int64_t>(value[i]) << (i * 8);
        }
        return result;
    }
    
    int64_t result = 0;
    result |= static_cast<int64_t>(value[0]);
    result |= static_cast<int64_t>(value[1]) << 8;
    result |= static_cast<int64_t>(value[2]) << 16;
    result |= static_cast<int64_t>(value[3]) << 24;
    result |= static_cast<int64_t>(value[4]) << 32;
    result |= static_cast<int64_t>(value[5]) << 40;
    result |= static_cast<int64_t>(value[6]) << 48;
    result |= static_cast<int64_t>(value[7]) << 56;
    return result;
}

float ImmediateOperand::getFloatValue() const {
    if (value.size() < 4) {
        return 0.0f;
    }
    
    float result;
    std::memcpy(&result, value.data(), 4);
    return result;
}

double ImmediateOperand::getDoubleValue() const {
    if (value.size() < 8) {
        return 0.0;
    }
    
    double result;
    std::memcpy(&result, value.data(), 8);
    return result;
}

std::unique_ptr<ImmediateOperand> ImmediateOperand::decode(const uint8_t* data, size_t& offset) {
    if (data == nullptr) {
        return nullptr;
    }
    
    uint8_t typeByte = data[offset++];
    uint8_t immType = typeByte & 0x3F; // Lower 6 bits
    
    // Determine the size of the immediate value
    size_t valueSize = 0;
    switch (immType) {
        case IMM_INT8:
            valueSize = 1;
            break;
        case IMM_INT16:
            valueSize = 2;
            break;
        case IMM_INT32:
        case IMM_FLOAT32:
            valueSize = 4;
            break;
        case IMM_INT64:
        case IMM_FLOAT64:
            valueSize = 8;
            break;
        case IMM_SYMBOL:
            // Symbol is a null-terminated string
            valueSize = 0;
            while (data[offset + valueSize] != 0) {
                valueSize++;
            }
            valueSize++; // Include null terminator
            break;
        default:
            // Unknown immediate type
            return nullptr;
    }
    
    // Copy the immediate value
    std::vector<uint8_t> value(data + offset, data + offset + valueSize);
    offset += valueSize;
    
    return std::make_unique<ImmediateOperand>(immType, value);
}

//
// MemoryOperand implementation
//

MemoryOperand::MemoryOperand(uint8_t type, const std::vector<uint8_t>& opData)
    : memType(type), data(opData) {
}

MemoryOperand::MemoryOperand(uint8_t regId)
    : memType(MEM_REG), data(1) {
    data[0] = regId;
}

MemoryOperand::MemoryOperand(uint8_t regId, int32_t disp)
    : memType(MEM_REG_DISP), data(5) {
    data[0] = regId;
    data[1] = disp & 0xFF;
    data[2] = (disp >> 8) & 0xFF;
    data[3] = (disp >> 16) & 0xFF;
    data[4] = (disp >> 24) & 0xFF;
}

MemoryOperand::MemoryOperand(uint8_t regId1, uint8_t regId2)
    : memType(MEM_REG_REG), data(2) {
    data[0] = regId1;
    data[1] = regId2;
}

MemoryOperand::MemoryOperand(uint8_t regId1, uint8_t regId2, uint8_t scale)
    : memType(MEM_REG_REG_SCALE), data(3) {
    data[0] = regId1;
    data[1] = regId2;
    data[2] = scale;
}

uint8_t MemoryOperand::getTypeByte() const {
    return OPERAND_MEMORY | memType;
}

std::vector<uint8_t> MemoryOperand::encode() const {
    std::vector<uint8_t> result;
    result.push_back(getTypeByte());
    result.insert(result.end(), data.begin(), data.end());
    return result;
}

std::string MemoryOperand::toString() const {
    std::ostringstream oss;
    
    oss << "[";
    
    switch (memType) {
        case MEM_DIRECT:
            // Direct memory access [addr]
            if (data.size() >= 4) {
                int32_t addr = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
                oss << "0x" << std::hex << addr;
            }
            break;
        case MEM_REG:
            // Register indirect [reg]
            if (data.size() >= 1) {
                oss << "R" << static_cast<int>(data[0]);
            }
            break;
        case MEM_REG_DISP:
            // Register + displacement [reg+disp]
            if (data.size() >= 5) {
                oss << "R" << static_cast<int>(data[0]);
                int32_t disp = data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24);
                if (disp > 0) {
                    oss << " + " << disp;
                } else if (disp < 0) {
                    oss << " - " << -disp;
                }
            }
            break;
        case MEM_REG_REG:
            // Register + register [reg1+reg2]
            if (data.size() >= 2) {
                oss << "R" << static_cast<int>(data[0]) << " + R" << static_cast<int>(data[1]);
            }
            break;
        case MEM_REG_REG_SCALE:
            // Register + scaled register [reg1+reg2*scale]
            if (data.size() >= 3) {
                oss << "R" << static_cast<int>(data[0]) << " + R" << static_cast<int>(data[1]);
                if (data[2] > 1) {
                    oss << "*" << static_cast<int>(data[2]);
                }
            }
            break;
        case MEM_REG_PRE_INC:
            // Pre-increment [++reg]
            if (data.size() >= 1) {
                oss << "++R" << static_cast<int>(data[0]);
            }
            break;
        case MEM_REG_PRE_DEC:
            // Pre-decrement [--reg]
            if (data.size() >= 1) {
                oss << "--R" << static_cast<int>(data[0]);
            }
            break;
        case MEM_REG_POST_INC:
            // Post-increment [reg++]
            if (data.size() >= 1) {
                oss << "R" << static_cast<int>(data[0]) << "++";
            }
            break;
        case MEM_REG_POST_DEC:
            // Post-decrement [reg--]
            if (data.size() >= 1) {
                oss << "R" << static_cast<int>(data[0]) << "--";
            }
            break;
        default:
            // Unknown memory operand type
            oss << "UNKNOWN";
            break;
    }
    
    oss << "]";
    
    return oss.str();
}

std::unique_ptr<Operand> MemoryOperand::clone() const {
    return std::make_unique<MemoryOperand>(memType, data);
}

uint8_t MemoryOperand::getMemType() const {
    return memType;
}

const std::vector<uint8_t>& MemoryOperand::getData() const {
    return data;
}

std::unique_ptr<MemoryOperand> MemoryOperand::decode(const uint8_t* data, size_t& offset) {
    if (data == nullptr) {
        return nullptr;
    }
    
    uint8_t typeByte = data[offset++];
    uint8_t memType = typeByte & 0x3F; // Lower 6 bits
    
    // Determine the size of the memory operand data
    size_t dataSize = 0;
    switch (memType) {
        case MEM_DIRECT:
            dataSize = 4; // 32-bit address
            break;
        case MEM_REG:
        case MEM_REG_PRE_INC:
        case MEM_REG_PRE_DEC:
        case MEM_REG_POST_INC:
        case MEM_REG_POST_DEC:
            dataSize = 1; // Register ID
            break;
        case MEM_REG_DISP:
            dataSize = 5; // Register ID + 32-bit displacement
            break;
        case MEM_REG_REG:
            dataSize = 2; // Two register IDs
            break;
        case MEM_REG_REG_SCALE:
            dataSize = 3; // Two register IDs + scale
            break;
        default:
            // Unknown memory operand type
            return nullptr;
    }
    
    // Copy the memory operand data
    std::vector<uint8_t> memData(data + offset, data + offset + dataSize);
    offset += dataSize;
    
    return std::make_unique<MemoryOperand>(memType, memData);
}

//
// VariableOperand implementation
//

VariableOperand::VariableOperand(uint8_t type, uint8_t id)
    : varType(type), varId(id) {
}

VariableOperand::VariableOperand(uint8_t id)
    : varType(VAR_DIRECT), varId(id) {
}

uint8_t VariableOperand::getTypeByte() const {
    return OPERAND_VARIABLE | varType;
}

std::vector<uint8_t> VariableOperand::encode() const {
    std::vector<uint8_t> result;
    result.push_back(getTypeByte());
    result.push_back(varId);
    return result;
}

std::string VariableOperand::toString() const {
    std::ostringstream oss;
    
    switch (varType) {
        case VAR_DIRECT:
            // Direct variable reference
            oss << "$" << static_cast<int>(varId);
            break;
        case VAR_ADDR:
            // Variable address
            oss << "&$" << static_cast<int>(varId);
            break;
        case VAR_ELEM:
            // Array element
            oss << "$" << static_cast<int>(varId) << "[idx]";
            break;
        case VAR_FIELD:
            // Structure field
            oss << "$" << static_cast<int>(varId) << ".field";
            break;
        default:
            // Unknown variable reference type
            oss << "$" << static_cast<int>(varId) << ".<unknown>";
            break;
    }
    
    return oss.str();
}

std::unique_ptr<Operand> VariableOperand::clone() const {
    return std::make_unique<VariableOperand>(varType, varId);
}

uint8_t VariableOperand::getVarType() const {
    return varType;
}

uint8_t VariableOperand::getVarId() const {
    return varId;
}

std::unique_ptr<VariableOperand> VariableOperand::decode(const uint8_t* data, size_t& offset) {
    if (data == nullptr) {
        return nullptr;
    }
    
    uint8_t typeByte = data[offset++];
    uint8_t varType = typeByte & 0x3F; // Lower 6 bits
    uint8_t varId = data[offset++];
    
    return std::make_unique<VariableOperand>(varType, varId);
}

} // namespace coil