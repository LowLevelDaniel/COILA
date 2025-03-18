#ifndef COIL_TARGET_X86_64_H
#define COIL_TARGET_X86_64_H

#include "target/target.h"

namespace coil {

/**
 * @brief x86-64 register identifiers
 */
enum X86_64Register : uint8_t {
    X86_64_RAX = 0,
    X86_64_RBX = 1,
    X86_64_RCX = 2,
    X86_64_RDX = 3,
    X86_64_RSI = 4,
    X86_64_RDI = 5,
    X86_64_RSP = 6,
    X86_64_RBP = 7,
    X86_64_R8  = 8,
    X86_64_R9  = 9,
    X86_64_R10 = 10,
    X86_64_R11 = 11,
    X86_64_R12 = 12,
    X86_64_R13 = 13,
    X86_64_R14 = 14,
    X86_64_R15 = 15,
    
    X86_64_XMM0  = 16,
    X86_64_XMM1  = 17,
    X86_64_XMM2  = 18,
    X86_64_XMM3  = 19,
    X86_64_XMM4  = 20,
    X86_64_XMM5  = 21,
    X86_64_XMM6  = 22,
    X86_64_XMM7  = 23,
    X86_64_XMM8  = 24,
    X86_64_XMM9  = 25,
    X86_64_XMM10 = 26,
    X86_64_XMM11 = 27,
    X86_64_XMM12 = 28,
    X86_64_XMM13 = 29,
    X86_64_XMM14 = 30,
    X86_64_XMM15 = 31,
    
    X86_64_RIP   = 32,
    X86_64_RFLAGS = 33
};

/**
 * @brief x86-64 register classes
 */
enum X86_64RegisterClass : uint8_t {
    X86_64_REG_CLASS_GP = 0,     // General-purpose registers
    X86_64_REG_CLASS_XMM = 1,    // XMM registers (SSE)
    X86_64_REG_CLASS_YMM = 2,    // YMM registers (AVX)
    X86_64_REG_CLASS_ZMM = 3,    // ZMM registers (AVX-512)
    X86_64_REG_CLASS_SPECIAL = 4 // Special registers
};

/**
 * @brief x86-64 feature flags
 */
enum X86_64Feature : uint32_t {
    X86_64_FEATURE_SSE  = (1 << 0),
    X86_64_FEATURE_SSE2 = (1 << 1),
    X86_64_FEATURE_SSE3 = (1 << 2),
    X86_64_FEATURE_SSSE3 = (1 << 3),
    X86_64_FEATURE_SSE4_1 = (1 << 4),
    X86_64_FEATURE_SSE4_2 = (1 << 5),
    X86_64_FEATURE_AVX = (1 << 6),
    X86_64_FEATURE_AVX2 = (1 << 7),
    X86_64_FEATURE_AVX512F = (1 << 8),
    X86_64_FEATURE_BMI1 = (1 << 9),
    X86_64_FEATURE_BMI2 = (1 << 10),
    X86_64_FEATURE_FMA = (1 << 11),
    X86_64_FEATURE_POPCNT = (1 << 12),
    X86_64_FEATURE_LZCNT = (1 << 13),
    X86_64_FEATURE_MOVBE = (1 << 14),
    X86_64_FEATURE_AES = (1 << 15),
    X86_64_FEATURE_PCLMUL = (1 << 16),
    X86_64_FEATURE_RDRAND = (1 << 17)
};

/**
 * @brief x86-64 target implementation
 */
class X86_64Target : public Target {
private:
    // Helper methods for instruction implementation
    void implementMathInstruction(Instruction& inst);
    void implementMemoryInstruction(Instruction& inst);
    void implementControlFlowInstruction(Instruction& inst);
    void implementBitInstruction(Instruction& inst);
    void implementVectorInstruction(Instruction& inst);
    void implementVariableInstruction(Instruction& inst);
    void implementFrameInstruction(Instruction& inst);

public:
    /**
     * @brief Construct a new X86_64Target
     * 
     * @param targetId Target ID
     * @param features Feature flags (default: basic x86-64)
     */
    X86_64Target(uint32_t targetId, uint32_t features = 0);
    
    /**
     * @brief Initialize register mappings
     */
    void initRegisterMappings();
    
    /**
     * @brief Transform an instruction for x86-64
     * 
     * @param inst Instruction to transform
     */
    void transformInstruction(Instruction& inst) override;
    
    /**
     * @brief Allocate registers for a function
     * 
     * @param func Function to allocate registers for
     */
    void allocateRegisters(Function& func) override;
    
    /**
     * @brief Generate function prologue for x86-64
     * 
     * @param func Function to generate prologue for
     * @return Generated instructions
     */
    std::vector<std::unique_ptr<Instruction>> generatePrologue(Function& func) override;
    
    /**
     * @brief Generate function epilogue for x86-64
     * 
     * @param func Function to generate epilogue for
     * @return Generated instructions
     */
    std::vector<std::unique_ptr<Instruction>> generateEpilogue(Function& func) override;
};

} // namespace coil

#endif // COIL_TARGET_X86_64_H