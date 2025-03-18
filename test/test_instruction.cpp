#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "core/instruction.h"
#include "core/operand.h"
#include "util/logger.h"

using namespace coil;

/**
 * @brief Test creating and encoding a simple instruction
 */
bool test_instruction_basic() {
    // Create a simple MATH ADD instruction
    Instruction inst(CAT_MATH, MATH_ADD);
    
    // Add operands
    inst.addOperand(std::make_unique<RegisterOperand>(REG_GP, REG_R0));
    inst.addOperand(std::make_unique<RegisterOperand>(REG_GP, REG_R1));
    inst.addOperand(std::make_unique<ImmediateOperand>(42));
    
    // Encode the instruction
    std::vector<uint8_t> encoded = inst.encode();
    
    // Verify the encoded instruction
    if (encoded.size() < 4) {
        std::cout << "Expected encoded instruction to be at least 4 bytes\n";
        return false;
    }
    
    // Check the opcode
    if (encoded[0] != 0x40) { // MATH ADD
        std::cout << "Expected opcode 0x40, got 0x" << std::hex << static_cast<int>(encoded[0]) << "\n";
        return false;
    }
    
    // Check the operand count
    if (encoded[1] != 3) {
        std::cout << "Expected operand count 3, got " << static_cast<int>(encoded[1]) << "\n";
        return false;
    }
    
    // Check that the instruction string contains the opcode and operands
    std::string str = inst.toString();
    if (str.find("MATH ADD") == std::string::npos) {
        std::cout << "Expected instruction string to contain 'MATH ADD', got '" << str << "'\n";
        return false;
    }
    
    if (str.find("R0") == std::string::npos || str.find("R1") == std::string::npos || str.find("42") == std::string::npos) {
        std::cout << "Expected instruction string to contain operands, got '" << str << "'\n";
        return false;
    }
    
    return true;
}

/**
 * @brief Test decoding an instruction
 */
bool test_instruction_decode() {
    // Create an instruction
    auto origInst = std::make_unique<Instruction>(CAT_MEM, MEM_MOV);
    origInst->addOperand(std::make_unique<RegisterOperand>(REG_GP, REG_R0));
    origInst->addOperand(std::make_unique<MemoryOperand>(REG_R1));
    
    // Encode the instruction
    std::vector<uint8_t> encoded = origInst->encode();
    
    // Decode the instruction
    size_t offset = 0;
    auto decodedInst = Instruction::decode(encoded.data(), offset);
    
    // Verify that the entire instruction was decoded
    if (offset != encoded.size()) {
        std::cout << "Expected to decode entire instruction, but offset is " << offset << " and size is " << encoded.size() << "\n";
        return false;
    }
    
    // Verify the decoded instruction
    if (decodedInst->getCategory() != CAT_MEM || decodedInst->getOperation() != MEM_MOV) {
        std::cout << "Expected decoded instruction to be MEM MOV\n";
        return false;
    }
    
    const auto& operands = decodedInst->getOperands();
    if (operands.size() != 2) {
        std::cout << "Expected decoded instruction to have 2 operands, got " << operands.size() << "\n";
        return false;
    }
    
    // Check the first operand
    const auto* regOp = dynamic_cast<const RegisterOperand*>(operands[0].get());
    if (!regOp || regOp->getRegId() != REG_R0) {
        std::cout << "Expected first operand to be register R0\n";
        return false;
    }
    
    // Check the second operand
    const auto* memOp = dynamic_cast<const MemoryOperand*>(operands[1].get());
    if (!memOp || memOp->getMemType() != MEM_REG) {
        std::cout << "Expected second operand to be memory operand with register indirect addressing\n";
        return false;
    }
    
    return true;
}

/**
 * @brief Test creating different types of operands
 */
bool test_operands() {
    // Test register operand
    auto regOp = std::make_unique<RegisterOperand>(REG_GP, REG_R0);
    if (regOp->getRegId() != REG_R0 || regOp->getRegType() != REG_GP) {
        std::cout << "Register operand properties do not match\n";
        return false;
    }
    
    // Test immediate operand (integer)
    auto immIntOp = std::make_unique<ImmediateOperand>(42);
    if (immIntOp->getInt32Value() != 42) {
        std::cout << "Immediate integer operand value does not match\n";
        return false;
    }
    
    // Test immediate operand (float)
    auto immFloatOp = std::make_unique<ImmediateOperand>(3.14159f);
    if (std::abs(immFloatOp->getFloatValue() - 3.14159f) > 1e-5f) {
        std::cout << "Immediate float operand value does not match\n";
        return false;
    }
    
    // Test immediate operand (symbol)
    auto immSymOp = std::make_unique<ImmediateOperand>("symbol_name");
    std::string str = immSymOp->toString();
    if (str != "symbol_name") {
        std::cout << "Immediate symbol operand string does not match, got '" << str << "'\n";
        return false;
    }
    
    // Test memory operand (register indirect)
    auto memRegOp = std::make_unique<MemoryOperand>(REG_R1);
    if (memRegOp->getMemType() != MEM_REG) {
        std::cout << "Memory operand type does not match\n";
        return false;
    }
    
    // Test memory operand (register + displacement)
    auto memRegDispOp = std::make_unique<MemoryOperand>(REG_R1, 4);
    if (memRegDispOp->getMemType() != MEM_REG_DISP) {
        std::cout << "Memory operand with displacement type does not match\n";
        return false;
    }
    
    // Test memory operand (register + register)
    auto memRegRegOp = std::make_unique<MemoryOperand>(REG_R1, REG_R2);
    if (memRegRegOp->getMemType() != MEM_REG_REG) {
        std::cout << "Memory operand with two registers type does not match\n";
        return false;
    }
    
    // Test memory operand (register + scaled register)
    auto memRegRegScaleOp = std::make_unique<MemoryOperand>(REG_R1, REG_R2, 4);
    if (memRegRegScaleOp->getMemType() != MEM_REG_REG_SCALE) {
        std::cout << "Memory operand with scaled register type does not match\n";
        return false;
    }
    
    // Test variable operand
    auto varOp = std::make_unique<VariableOperand>(10);
    if (varOp->getVarId() != 10 || varOp->getVarType() != VAR_DIRECT) {
        std::cout << "Variable operand properties do not match\n";
        return false;
    }
    
    return true;
}

/**
 * @brief Test extended data in instructions
 */
bool test_instruction_extended_data() {
    // Create an instruction with extended data
    Instruction inst(CAT_CF, CF_BRC);
    
    // Add operand (branch target)
    inst.addOperand(std::make_unique<ImmediateOperand>("label"));
    
    // Add extended data (condition code)
    std::vector<uint8_t> extData = { COND_EQ, 0 };
    inst.setExtendedData(extData);
    
    // Encode the instruction
    std::vector<uint8_t> encoded = inst.encode();
    
    // Verify the encoded instruction
    if (encoded.size() < 4) {
        std::cout << "Expected encoded instruction to be at least 4 bytes\n";
        return false;
    }
    
    // Check the extended data size
    if (encoded[2] != 2 || encoded[3] != 0) {
        std::cout << "Expected extended data size to be 2, got " 
                  << static_cast<int>(encoded[2]) + (static_cast<int>(encoded[3]) << 8) << "\n";
        return false;
    }
    
    // Decode the instruction
    size_t offset = 0;
    auto decodedInst = Instruction::decode(encoded.data(), offset);
    
    // Verify the extended data
    const auto& decodedExtData = decodedInst->getExtendedData();
    if (decodedExtData.size() != 2 || decodedExtData[0] != COND_EQ || decodedExtData[1] != 0) {
        std::cout << "Decoded extended data does not match\n";
        return false;
    }
    
    return true;
}

/**
 * @brief Run all instruction tests
 */
bool test_instruction() {
    std::cout << "Testing instructions...\n";
    
    bool success = true;
    
    success &= test_instruction_basic();
    success &= test_instruction_decode();
    success &= test_operands();
    success &= test_instruction_extended_data();
    
    if (success) {
        std::cout << "All instruction tests passed.\n";
    } else {
        std::cout << "Some instruction tests failed.\n";
    }
    
    return success;
}