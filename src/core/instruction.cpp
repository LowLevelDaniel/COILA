#include "core/instruction.h"
#include <sstream>
#include <iomanip>

namespace coil {

Instruction::Instruction(uint8_t cat, uint8_t op)
    : category(cat), operation(op) {
}

void Instruction::addOperand(OperandPtr op) {
    if (op) {
        operands.push_back(std::move(op));
    }
}

void Instruction::setExtendedData(const std::vector<uint8_t>& data) {
    extendedData = data;
}

uint8_t Instruction::getCategory() const {
    return category;
}

uint8_t Instruction::getOperation() const {
    return operation;
}

uint8_t Instruction::getOpcode() const {
    return category | operation;
}

const std::vector<OperandPtr>& Instruction::getOperands() const {
    return operands;
}

const std::vector<uint8_t>& Instruction::getExtendedData() const {
    return extendedData;
}

std::vector<uint8_t> Instruction::encode() const {
    std::vector<uint8_t> result;
    
    // Encode the instruction header
    result.push_back(getOpcode());
    result.push_back(static_cast<uint8_t>(operands.size()));
    
    // Encode the extended data size (little-endian)
    uint16_t extSize = static_cast<uint16_t>(extendedData.size());
    result.push_back(extSize & 0xFF);
    result.push_back((extSize >> 8) & 0xFF);
    
    // Encode the operands
    for (const auto& op : operands) {
        std::vector<uint8_t> opData = op->encode();
        result.insert(result.end(), opData.begin(), opData.end());
    }
    
    // Append the extended data
    result.insert(result.end(), extendedData.begin(), extendedData.end());
    
    return result;
}

std::unique_ptr<Instruction> Instruction::decode(const uint8_t* data, size_t& offset) {
    // Check if we have enough data for the instruction header
    if (data == nullptr) {
        return nullptr;
    }
    
    // Read the instruction header
    uint8_t opcode = data[offset++];
    uint8_t operandCount = data[offset++];
    uint16_t extDataSize = data[offset] | (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;
    
    // Create the instruction
    uint8_t category = opcode & 0xE0; // Upper 3 bits
    uint8_t operation = opcode & 0x1F; // Lower 5 bits
    auto instruction = std::make_unique<Instruction>(category, operation);
    
    // Decode the operands
    for (uint8_t i = 0; i < operandCount; i++) {
        auto operand = Operand::decode(data, offset);
        if (operand) {
            instruction->addOperand(std::move(operand));
        }
    }
    
    // Read the extended data
    if (extDataSize > 0) {
        instruction->extendedData.assign(data + offset, data + offset + extDataSize);
        offset += extDataSize;
    }
    
    return instruction;
}

std::string Instruction::toString() const {
    std::ostringstream oss;
    
    // Get the instruction category name
    const char* categoryName = "UNKNOWN";
    switch (category) {
        case CAT_CF:    categoryName = "CF"; break;
        case CAT_MEM:   categoryName = "MEM"; break;
        case CAT_MATH:  categoryName = "MATH"; break;
        case CAT_BIT:   categoryName = "BIT"; break;
        case CAT_VEC:   categoryName = "VEC"; break;
        case CAT_ATM:   categoryName = "ATM"; break;
        case CAT_VAR:   categoryName = "VAR"; break;
        case CAT_FRAME: categoryName = "FRAME"; break;
    }
    
    // Get the instruction operation name
    std::string opName = "UNKNOWN";
    switch (category) {
        case CAT_CF:
            switch (operation) {
                case CF_BR:     opName = "BR"; break;
                case CF_BRC:    opName = "BRC"; break;
                case CF_CALL:   opName = "CALL"; break;
                case CF_RET:    opName = "RET"; break;
                case CF_INT:    opName = "INT"; break;
                case CF_IRET:   opName = "IRET"; break;
                case CF_HLT:    opName = "HLT"; break;
                case CF_SYSC:   opName = "SYSC"; break;
                case CF_TRAP:   opName = "TRAP"; break;
                case CF_WFE:    opName = "WFE"; break;
                case CF_SEV:    opName = "SEV"; break;
                case CF_FENCE:  opName = "FENCE"; break;
                case CF_YIELD:  opName = "YIELD"; break;
                case CF_SWITCH: opName = "SWITCH"; break;
                case CF_NOP:    opName = "NOP"; break;
            }
            break;
        case CAT_MEM:
            switch (operation) {
                case MEM_MOV:      opName = "MOV"; break;
                case MEM_PUSH:     opName = "PUSH"; break;
                case MEM_POP:      opName = "POP"; break;
                case MEM_LOAD:     opName = "LOAD"; break;
                case MEM_STORE:    opName = "STORE"; break;
                case MEM_PREFETCH: opName = "PREFETCH"; break;
                case MEM_EXCHANGE: opName = "EXCHANGE"; break;
                case MEM_COMPARE:  opName = "COMPARE"; break;
                case MEM_TEST:     opName = "TEST"; break;
                case MEM_FILL:     opName = "FILL"; break;
                case MEM_COPY:     opName = "COPY"; break;
                case MEM_ZERO:     opName = "ZERO"; break;
                case MEM_PUSH_STATE: opName = "PUSH_STATE"; break;
                case MEM_POP_STATE:  opName = "POP_STATE"; break;
                case MEM_OUT:      opName = "OUT"; break;
                case MEM_IN:       opName = "IN"; break;
            }
            break;
        case CAT_MATH:
            switch (operation) {
                case MATH_ADD:    opName = "ADD"; break;
                case MATH_SUB:    opName = "SUB"; break;
                case MATH_MUL:    opName = "MUL"; break;
                case MATH_DIV:    opName = "DIV"; break;
                case MATH_MOD:    opName = "MOD"; break;
                case MATH_NEG:    opName = "NEG"; break;
                case MATH_INC:    opName = "INC"; break;
                case MATH_DEC:    opName = "DEC"; break;
                case MATH_ABS:    opName = "ABS"; break;
                case MATH_SQRT:   opName = "SQRT"; break;
                case MATH_MIN:    opName = "MIN"; break;
                case MATH_MAX:    opName = "MAX"; break;
                case MATH_FMA:    opName = "FMA"; break;
                case MATH_ROUND:  opName = "ROUND"; break;
                case MATH_FLOOR:  opName = "FLOOR"; break;
                case MATH_CEIL:   opName = "CEIL"; break;
                case MATH_TRUNC:  opName = "TRUNC"; break;
            }
            break;
        case CAT_BIT:
            switch (operation) {
                case BIT_AND:    opName = "AND"; break;
                case BIT_OR:     opName = "OR"; break;
                case BIT_XOR:    opName = "XOR"; break;
                case BIT_NOT:    opName = "NOT"; break;
                case BIT_ANDN:   opName = "ANDN"; break;
                case BIT_ORN:    opName = "ORN"; break;
                case BIT_XNOR:   opName = "XNOR"; break;
                case BIT_SHL:    opName = "SHL"; break;
                case BIT_SHR:    opName = "SHR"; break;
                case BIT_SAR:    opName = "SAR"; break;
                case BIT_ROL:    opName = "ROL"; break;
                case BIT_ROR:    opName = "ROR"; break;
                case BIT_RCL:    opName = "RCL"; break;
                case BIT_RCR:    opName = "RCR"; break;
                case BIT_BSWAP:  opName = "BSWAP"; break;
                case BIT_BITREV: opName = "BITREV"; break;
                case BIT_CLZ:    opName = "CLZ"; break;
                case BIT_CTZ:    opName = "CTZ"; break;
                case BIT_POPCNT: opName = "POPCNT"; break;
                case BIT_PARITY: opName = "PARITY"; break;
                case BIT_EXTRACT: opName = "EXTRACT"; break;
                case BIT_INSERT:  opName = "INSERT"; break;
                case BIT_SET:    opName = "SET"; break;
                case BIT_CLR:    opName = "CLR"; break;
                case BIT_TST:    opName = "TST"; break;
                case BIT_TGL:    opName = "TGL"; break;
                case BIT_CMP:    opName = "CMP"; break;
            }
            break;
        case CAT_VAR:
            switch (operation) {
                case VAR_DECL:   opName = "DECL"; break;
                case VAR_PMT:    opName = "PMT"; break;
                case VAR_DMT:    opName = "DMT"; break;
                case VAR_DLT:    opName = "DLT"; break;
                case VAR_ALIAS:  opName = "ALIAS"; break;
            }
            break;
        case CAT_FRAME:
            switch (operation) {
                case FRAME_ENTER: opName = "ENTER"; break;
                case FRAME_LEAVE: opName = "LEAVE"; break;
                case FRAME_SAVE:  opName = "SAVE"; break;
                case FRAME_REST:  opName = "REST"; break;
            }
            break;
    }
    
    // Build the instruction string
    oss << categoryName << " " << opName;
    
    // Add the operands
    if (!operands.empty()) {
        oss << " ";
        for (size_t i = 0; i < operands.size(); i++) {
            if (i > 0) {
                oss << ", ";
            }
            oss << operands[i]->toString();
        }
    }
    
    // Add the extended data if present
    if (!extendedData.empty()) {
        oss << " ; Extended data: ";
        for (uint8_t byte : extendedData) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
        }
    }
    
    return oss.str();
}

} // namespace coil