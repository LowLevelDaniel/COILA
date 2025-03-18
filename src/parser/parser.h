#ifndef COIL_PARSER_PARSER_H
#define COIL_PARSER_PARSER_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "parser/lexer.h"
#include "core/instruction.h"
#include "core/operand.h"
#include "util/diagnostic.h"
#include "binary/cof.h"

namespace coil {

/**
 * @brief Function definition
 */
class Function {
private:
    std::string name;        // Function name
    std::vector<std::unique_ptr<Instruction>> instructions; // Instructions
    std::vector<uint8_t> variableTypes;     // Variable types
    std::vector<std::vector<uint8_t>> variableInitValues; // Variable initial values
    std::map<std::string, size_t> labels;   // Label -> instruction index mapping
    std::vector<std::pair<size_t, std::string>> labelRefs; // Instruction index -> label reference
    uint16_t flags;          // Function flags

public:
    /**
     * @brief Construct a new Function
     * 
     * @param funcName Function name
     * @param funcFlags Function flags
     */
    Function(const std::string& funcName, uint16_t funcFlags = 0);
    
    /**
     * @brief Get the function name
     * 
     * @return Function name
     */
    const std::string& getName() const;
    
    /**
     * @brief Get the function flags
     * 
     * @return Function flags
     */
    uint16_t getFlags() const;
    
    /**
     * @brief Add an instruction
     * 
     * @param instruction Instruction to add
     * @return Instruction index
     */
    size_t addInstruction(std::unique_ptr<Instruction> instruction);
    
    /**
     * @brief Get all instructions
     * 
     * @return Vector of instructions
     */
    const std::vector<std::unique_ptr<Instruction>>& getInstructions() const;
    
    /**
     * @brief Add a label
     * 
     * @param labelName Label name
     * @param instructionIndex Instruction index
     * @return true if added successfully, false if label already exists
     */
    bool addLabel(const std::string& labelName, size_t instructionIndex);
    
    /**
     * @brief Add a label reference
     * 
     * @param instructionIndex Instruction index
     * @param labelName Referenced label name
     */
    void addLabelRef(size_t instructionIndex, const std::string& labelName);
    
    /**
     * @brief Resolve all label references
     * 
     * @param symbols Symbol table (for global labels)
     * @param symbolOverrides Symbol overrides (for linking)
     * @return true if all references resolved, false otherwise
     */
    bool resolveLabels(const std::vector<std::unique_ptr<Symbol>>& symbols, 
                      const std::map<std::string, std::string>& symbolOverrides = {});
    
    /**
     * @brief Set the type for a variable
     * 
     * @param varId Variable ID
     * @param typeId Type ID
     */
    void setVariableType(uint8_t varId, uint8_t typeId);
    
    /**
     * @brief Get the type for a variable
     * 
     * @param varId Variable ID
     * @return Type ID, or 0 if not set
     */
    uint8_t getVariableType(uint8_t varId) const;
    
    /**
     * @brief Set the initial value for a variable
     * 
     * @param varId Variable ID
     * @param value Initial value
     */
    void setVariableInitValue(uint8_t varId, const std::vector<uint8_t>& value);
    
    /**
     * @brief Get the initial value for a variable
     * 
     * @param varId Variable ID
     * @return Initial value, or empty vector if not set
     */
    const std::vector<uint8_t>& getVariableInitValue(uint8_t varId) const;
};

/**
 * @brief ABI definition
 */
struct AbiDefinition {
    std::string name;                // ABI name
    std::vector<uint8_t> argRegs;    // Argument registers
    std::vector<uint8_t> retRegs;    // Return registers
    std::vector<uint8_t> preservedRegs; // Preserved registers
    std::vector<uint8_t> volatileRegs;  // Volatile registers
    uint32_t stackAlign;             // Stack alignment
    
    AbiDefinition(const std::string& abiName) : name(abiName), stackAlign(16) {}
};

/**
 * @brief Module containing multiple functions and sections
 */
class Module {
private:
    std::string name;        // Module name
    std::vector<std::unique_ptr<Function>> functions; // Functions
    std::map<std::string, size_t> functionMap; // Function name -> index mapping
    std::map<std::string, AbiDefinition> abiDefinitions; // ABI definitions
    std::map<std::string, std::string> directives; // Directives
    std::string currentSection;      // Current section name
    uint32_t currentSectionType;     // Current section type
    uint32_t currentSectionFlags;    // Current section flags
    uint32_t currentTargetId;        // Current target architecture ID

public:
    /**
     * @brief Construct a new Module
     * 
     * @param moduleName Module name
     */
    Module(const std::string& moduleName);
    
    /**
     * @brief Get the module name
     * 
     * @return Module name
     */
    const std::string& getName() const;
    
    /**
     * @brief Add a function
     * 
     * @param function Function to add
     * @return true if added successfully, false if function already exists
     */
    bool addFunction(std::unique_ptr<Function> function);
    
    /**
     * @brief Get a function by name
     * 
     * @param name Function name
     * @return Function pointer, or nullptr if not found
     */
    Function* getFunctionByName(const std::string& name);
    
    /**
     * @brief Get all functions
     * 
     * @return Vector of functions
     */
    const std::vector<std::unique_ptr<Function>>& getFunctions() const;
    
    /**
     * @brief Add an ABI definition
     * 
     * @param name ABI name
     * @param def ABI definition
     * @return true if added successfully, false if ABI already exists
     */
    bool addAbiDefinition(const std::string& name, const AbiDefinition& def);
    
    /**
     * @brief Get an ABI definition by name
     * 
     * @param name ABI name
     * @return ABI definition, or nullptr if not found
     */
    const AbiDefinition* getAbiDefinition(const std::string& name) const;
    
    /**
     * @brief Add a directive
     * 
     * @param name Directive name
     * @param value Directive value
     */
    void addDirective(const std::string& name, const std::string& value);
    
    /**
     * @brief Get a directive value
     * 
     * @param name Directive name
     * @return Directive value, or empty string if not found
     */
    std::string getDirective(const std::string& name) const;
    
    /**
     * @brief Set the current section
     * 
     * @param name Section name
     * @param type Section type
     * @param flags Section flags
     */
    void setCurrentSection(const std::string& name, uint32_t type, uint32_t flags);
    
    /**
     * @brief Get the current section name
     * 
     * @return Current section name
     */
    const std::string& getCurrentSection() const;
    
    /**
     * @brief Get the current section type
     * 
     * @return Current section type
     */
    uint32_t getCurrentSectionType() const;
    
    /**
     * @brief Get the current section flags
     * 
     * @return Current section flags
     */
    uint32_t getCurrentSectionFlags() const;
    
    /**
     * @brief Set the current target architecture
     * 
     * @param targetId Target architecture ID
     */
    void setCurrentTargetId(uint32_t targetId);
    
    /**
     * @brief Get the current target architecture ID
     * 
     * @return Current target architecture ID
     */
    uint32_t getCurrentTargetId() const;
    
    /**
     * @brief Generate a COF file from this module
     * 
     * @return Generated COF file
     */
    std::unique_ptr<CofFile> generateCof();
};

/**
 * @brief Parser for COIL assembly
 * 
 * Parses COIL assembly into a module.
 */
class Parser {
private:
    std::vector<Token> tokens;   // Tokens
    size_t position;             // Current position in tokens
    DiagnosticEngine& diag;      // Diagnostics
    std::unique_ptr<Module> module; // Current module
    
    // Helper methods
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool isAtEnd() const;
    
    void consume(TokenType type, const std::string& message);
    
    // Parsing methods
    void parseModule();
    void parseDirective();
    void parseSection();
    void parseFunction();
    void parseAbi();
    void parseLabel();
    void parseInstruction();
    std::unique_ptr<Instruction> parseInstructionBody();
    
    // Operand parsing
    std::unique_ptr<Operand> parseOperand();
    std::unique_ptr<Operand> parseRegisterOperand();
    std::unique_ptr<Operand> parseVariableOperand();
    std::unique_ptr<Operand> parseImmediateOperand();
    std::unique_ptr<Operand> parseMemoryOperand();
    
    // Type parsing
    uint16_t parseTypeSpecifier();
    
    void error(const std::string& message);
    void error(const Token& token, const std::string& message);
    
public:
    /**
     * @brief Construct a new Parser
     * 
     * @param sourceTokens Tokens to parse
     * @param diagnostics Diagnostic engine
     */
    Parser(std::vector<Token> sourceTokens, DiagnosticEngine& diagnostics);
    
    /**
     * @brief Parse the tokens into a module
     * 
     * @return Parsed module
     */
    std::unique_ptr<Module> parse();
};

} // namespace coil

#endif // COIL_PARSER_PARSER_H