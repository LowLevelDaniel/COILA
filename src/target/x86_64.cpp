#include "target/x86_64.h"
#include "core/instruction.h"
#include "util/logger.h"

namespace coil {

X86_64Target::X86_64Target(uint32_t targetId, uint32_t features)
    : Target(targetId, 0, ARCH_X86_64, 64, 0, "x86-64") {
    
    // Set features
    this->features = features;
    
    // Initialize register mappings
    initRegisterMappings();
}

void X86_64Target::initRegisterMappings() {
    // Register mappings based on the COIL specification
    
    // General-purpose registers
    addRegisterMapping({REG_R0, X86_64_RAX, X86_64_REG_CLASS_GP});  // R0 -> RAX
    addRegisterMapping({REG_R1, X86_64_RBX, X86_64_REG_CLASS_GP});  // R1 -> RBX
    addRegisterMapping({REG_R2, X86_64_RCX, X86_64_REG_CLASS_GP});  // R2 -> RCX
    addRegisterMapping({REG_R3, X86_64_RDX, X86_64_REG_CLASS_GP});  // R3 -> RDX
    addRegisterMapping({REG_R4, X86_64_RDI, X86_64_REG_CLASS_GP});  // R4 -> RDI
    addRegisterMapping({REG_R5, X86_64_RSI, X86_64_REG_CLASS_GP});  // R5 -> RSI
    addRegisterMapping({REG_R6, X86_64_R8, X86_64_REG_CLASS_GP});   // R6 -> R8
    addRegisterMapping({REG_R7, X86_64_R9, X86_64_REG_CLASS_GP});   // R7 -> R9
    addRegisterMapping({REG_R8, X86_64_R10, X86_64_REG_CLASS_GP});  // R8 -> R10
    addRegisterMapping({REG_R9, X86_64_R11, X86_64_REG_CLASS_GP});  // R9 -> R11
    addRegisterMapping({REG_R10, X86_64_R12, X86_64_REG_CLASS_GP}); // R10 -> R12
    addRegisterMapping({REG_R11, X86_64_R13, X86_64_REG_CLASS_GP}); // R11 -> R13
    addRegisterMapping({REG_R12, X86_64_R14, X86_64_REG_CLASS_GP}); // R12 -> R14
    addRegisterMapping({REG_R13, X86_64_R15, X86_64_REG_CLASS_GP}); // R13 -> R15
    addRegisterMapping({REG_R14, X86_64_RSP, X86_64_REG_CLASS_GP}); // R14 -> RSP
    addRegisterMapping({REG_R15, X86_64_RBP, X86_64_REG_CLASS_GP}); // R15 -> RBP
    
    // Floating-point registers (XMM)
    for (uint8_t i = 0; i < 16; i++) {
        addRegisterMapping({static_cast<uint8_t>(REG_F0 + i), 
                         static_cast<uint8_t>(X86_64_XMM0 + i), 
                         X86_64_REG_CLASS_XMM});
    }
    
    // Vector registers (same as XMM for now, but with YMM/ZMM class for AVX/AVX-512)
    for (uint8_t i = 0; i < 16; i++) {
        uint8_t regClass = X86_64_REG_CLASS_XMM;
        if (features & X86_64_FEATURE_AVX) {
            regClass = X86_64_REG_CLASS_YMM;
        }
        if (features & X86_64_FEATURE_AVX512F) {
            regClass = X86_64_REG_CLASS_ZMM;
        }
        
        addRegisterMapping({static_cast<uint8_t>(REG_V0 + i), 
                         static_cast<uint8_t>(X86_64_XMM0 + i), 
                         regClass});
    }
    
    // Special registers
    addRegisterMapping({REG_PC, X86_64_RIP, X86_64_REG_CLASS_SPECIAL});      // PC -> RIP
    addRegisterMapping({REG_FLAGS, X86_64_RFLAGS, X86_64_REG_CLASS_SPECIAL}); // FLAGS -> RFLAGS
}

void X86_64Target::transformInstruction(Instruction& inst) {
    switch (inst.getCategory()) {
        case CAT_MATH:
            implementMathInstruction(inst);
            break;
        case CAT_MEM:
            implementMemoryInstruction(inst);
            break;
        case CAT_CF:
            implementControlFlowInstruction(inst);
            break;
        case CAT_BIT:
            implementBitInstruction(inst);
            break;
        case CAT_VEC:
            implementVectorInstruction(inst);
            break;
        case CAT_VAR:
            implementVariableInstruction(inst);
            break;
        case CAT_FRAME:
            implementFrameInstruction(inst);
            break;
        default:
            // Unknown category, leave as-is
            LOG_WARNING("Unknown instruction category: " + std::to_string(inst.getCategory()));
            break;
    }
}

void X86_64Target::allocateRegisters(Function& func) {
    // TODO: Implement register allocation
    // For now, just use the fixed mapping
}

std::vector<std::unique_ptr<Instruction>> X86_64Target::generatePrologue(Function& func) {
    // Generate standard x86-64 function prologue
    std::vector<std::unique_ptr<Instruction>> prologue;
    
    // FRAME ENTER -> push rbp, mov rbp, rsp
    auto pushRbp = std::make_unique<Instruction>(CAT_MEM, MEM_PUSH);
    pushRbp->addOperand(std::make_unique<RegisterOperand>(REG_GP, REG_R15));
    prologue.push_back(std::move(pushRbp));
    
    auto movRbpRsp = std::make_unique<Instruction>(CAT_MEM, MEM_MOV);
    movRbpRsp->addOperand(std::make_unique<RegisterOperand>(REG_GP, REG_R15));
    movRbpRsp->addOperand(std::make_unique<RegisterOperand>(REG_GP, REG_R14));
    prologue.push_back(std::move(movRbpRsp));
    
    // If there are callee-saved registers, push them
    // TODO: Add register saving based on function needs
    
    return prologue;
}

std::vector<std::unique_ptr<Instruction>> X86_64Target::generateEpilogue(Function& func) {
    // Generate standard x86-64 function epilogue
    std::vector<std::unique_ptr<Instruction>> epilogue;
    
    // If there are callee-saved registers, pop them
    // TODO: Add register restoring based on function needs
    
    // FRAME LEAVE -> leave (equivalent to: mov rsp, rbp; pop rbp)
    auto movRspRbp = std::make_unique<Instruction>(CAT_MEM, MEM_MOV);
    movRspRbp->addOperand(std::make_unique<RegisterOperand>(REG_GP, REG_R14));
    movRspRbp->addOperand(std::make_unique<RegisterOperand>(REG_GP, REG_R15));
    epilogue.push_back(std::move(movRspRbp));
    
    auto popRbp = std::make_unique<Instruction>(CAT_MEM, MEM_POP);
    popRbp->addOperand(std::make_unique<RegisterOperand>(REG_GP, REG_R15));
    epilogue.push_back(std::move(popRbp));
    
    // CF RET -> ret
    auto ret = std::make_unique<Instruction>(CAT_CF, CF_RET);
    epilogue.push_back(std::move(ret));
    
    return epilogue;
}

void X86_64Target::implementMathInstruction(Instruction& inst) {
    // For now, just a simple implementation
    // TODO: Implement full math instruction handling
}

void X86_64Target::implementMemoryInstruction(Instruction& inst) {
    // For now, just a simple implementation
    // TODO: Implement full memory instruction handling
}

void X86_64Target::implementControlFlowInstruction(Instruction& inst) {
    // For now, just a simple implementation
    // TODO: Implement full control flow instruction handling
}

void X86_64Target::implementBitInstruction(Instruction& inst) {
    // For now, just a simple implementation
    // TODO: Implement full bit instruction handling
}

void X86_64Target::implementVectorInstruction(Instruction& inst) {
    // For now, just a simple implementation
    // TODO: Implement full vector instruction handling
}

void X86_64Target::implementVariableInstruction(Instruction& inst) {
    // For now, just a simple implementation
    // TODO: Implement full variable instruction handling
}

void X86_64Target::implementFrameInstruction(Instruction& inst) {
    // For now, just a simple implementation
    // TODO: Implement full frame instruction handling
}

} // namespace coil