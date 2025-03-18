#include "parser/parser.h"
#include "util/logger.h"
#include <sstream>
#include <unordered_map>

namespace coil {

// Function implementation
Function::Function(const std::string& funcName, uint16_t funcFlags)
    : name(funcName), flags(funcFlags) {
    // Initialize variable arrays with a reasonable capacity
    variableTypes.resize(16, 0);  // Space for 16 variables initially
    variableInitValues.resize(16);
}

const std::string& Function::getName() const {
    return name;
}

uint16_t Function::getFlags() const {
    return flags;
}

size_t Function::addInstruction(std::unique_ptr<Instruction> instruction) {
    size_t index = instructions.size();
    instructions.push_back(std::move(instruction));
    return index;
}

const std::vector<std::unique_ptr<Instruction>>& Function::getInstructions() const {
    return instructions;
}

bool Function::addLabel(const std::string& labelName, size_t instructionIndex) {
    auto result = labels.insert({labelName, instructionIndex});
    return result.second; // true if inserted, false if already exists
}

void Function::addLabelRef(size_t instructionIndex, const std::string& labelName) {
    labelRefs.push_back({instructionIndex, labelName});
}

bool Function::resolveLabels(const std::vector<std::unique_ptr<Symbol>>& symbols,
                           const std::map<std::string, std::string>& symbolOverrides) {
    bool success = true;
    
    // Build a map of global symbols for quick lookup
    std::map<std::string, uint64_t> globalSymbols;
    for (const auto& symbol : symbols) {
        if (symbol->isGlobal() || symbol->isFunction()) {
            globalSymbols[symbol->getName()] = symbol->getValue();
        }
    }
    
    // Apply symbol overrides
    for (const auto& [symName, replName] : symbolOverrides) {
        auto it = globalSymbols.find(replName);
        if (it != globalSymbols.end()) {
            globalSymbols[symName] = it->second;
        }
    }
    
    // Resolve label references
    for (const auto& [instIndex, labelName] : labelRefs) {
        // Look for a local label first
        auto localIt = labels.find(labelName);
        if (localIt != labels.end()) {
            // Local label found, update the instruction
            // TODO: Update instruction operand with label address
            continue;
        }
        
        // Look for a global symbol
        auto globalIt = globalSymbols.find(labelName);
        if (globalIt != globalSymbols.end()) {
            // Global symbol found, update the instruction
            // TODO: Update instruction operand with symbol address
            continue;
        }
        
        // Label not found
        LOG_ERROR("Unresolved label reference: " + labelName);
        success = false;
    }
    
    return success;
}

void Function::setVariableType(uint8_t varId, uint8_t typeId) {
    // Ensure the vector is large enough
    if (varId >= variableTypes.size()) {
        variableTypes.resize(varId + 1, 0);
        variableInitValues.resize(varId + 1);
    }
    
    variableTypes[varId] = typeId;
}

uint8_t Function::getVariableType(uint8_t varId) const {
    if (varId < variableTypes.size()) {
        return variableTypes[varId];
    }
    return 0;
}

void Function::setVariableInitValue(uint8_t varId, const std::vector<uint8_t>& value) {
    // Ensure the vector is large enough
    if (varId >= variableInitValues.size()) {
        variableTypes.resize(varId + 1, 0);
        variableInitValues.resize(varId + 1);
    }
    
    variableInitValues[varId] = value;
}

const std::vector<uint8_t>& Function::getVariableInitValue(uint8_t varId) const {
    static const std::vector<uint8_t> emptyValue;
    
    if (varId < variableInitValues.size()) {
        return variableInitValues[varId];
    }
    
    return emptyValue;
}

// Module implementation
Module::Module(const std::string& moduleName)
    : name(moduleName), currentSectionType(0), currentSectionFlags(0), currentTargetId(0) {
    // Set default section
    currentSection = "text";
}

const std::string& Module::getName() const {
    return name;
}

bool Module::addFunction(std::unique_ptr<Function> function) {
    if (!function) {
        return false;
    }
    
    const std::string& funcName = function->getName();
    
    // Check if function already exists
    if (functionMap.find(funcName) != functionMap.end()) {
        return false;
    }
    
    // Add function to the map and vector
    size_t index = functions.size();
    functionMap[funcName] = index;
    functions.push_back(std::move(function));
    
    return true;
}

Function* Module::getFunctionByName(const std::string& name) {
    auto it = functionMap.find(name);
    if (it != functionMap.end()) {
        return functions[it->second].get();
    }
    return nullptr;
}

const std::vector<std::unique_ptr<Function>>& Module::getFunctions() const {
    return functions;
}

bool Module::addAbiDefinition(const std::string& name, const AbiDefinition& def) {
    auto result = abiDefinitions.insert({name, def});
    return result.second; // true if inserted, false if already exists
}

const AbiDefinition* Module::getAbiDefinition(const std::string& name) const {
    auto it = abiDefinitions.find(name);
    if (it != abiDefinitions.end()) {
        return &it->second;
    }
    return nullptr;
}

void Module::addDirective(const std::string& name, const std::string& value) {
    directives[name] = value;
}

std::string Module::getDirective(const std::string& name) const {
    auto it = directives.find(name);
    if (it != directives.end()) {
        return it->second;
    }
    return "";
}

void Module::setCurrentSection(const std::string& name, uint32_t type, uint32_t flags) {
    currentSection = name;
    currentSectionType = type;
    currentSectionFlags = flags;
}

const std::string& Module::getCurrentSection() const {
    return currentSection;
}

uint32_t Module::getCurrentSectionType() const {
    return currentSectionType;
}

uint32_t Module::getCurrentSectionFlags() const {
    return currentSectionFlags;
}

void Module::setCurrentTargetId(uint32_t targetId) {
    currentTargetId = targetId;
}

uint32_t Module::getCurrentTargetId() const {
    return currentTargetId;
}

std::unique_ptr<CofFile> Module::generateCof() {
    // Create a new COF file
    auto cof = std::make_unique<CofFile>();
    
    // Add target (use x86-64 for now)
    uint32_t targetId = cof->addTarget(ARCH_X86_64, 0, "x86-64");
    
    // Add sections
    Section& textSection = cof->addSection("text", SECTION_CODE, SECTION_FLAG_EXEC | SECTION_FLAG_ALLOC);
    Section& dataSection = cof->addSection("data", SECTION_DATA, SECTION_FLAG_ALLOC);
    
    // Add symbols and code
    for (const auto& function : functions) {
        // Add function symbol
        uint32_t symbolIndex = cof->addSymbol(function->getName(), 
                                             0, // textSection index
                                             0, // Value (offset) - will be updated
                                             0, // Size - will be updated
                                             SYMBOL_FUNCTION,
                                             SYMBOL_FLAG_GLOBAL,
                                             targetId);
        
        // Add function code
        for (const auto& instruction : function->getInstructions()) {
            // For now, just add all instructions to the text section
            // This should be more sophisticated based on sections in the future
            textSection.addInstruction(instruction->clone());
        }
    }
    
    // Finalize sections (convert instructions to binary)
    textSection.finalize();
    
    return cof;
}

// Parser implementation
Parser::Parser(std::vector<Token> sourceTokens, DiagnosticEngine& diagnostics)
    : tokens(std::move(sourceTokens)), position(0), diag(diagnostics) {
    // Create a default module
    module = std::make_unique<Module>("default");
}

std::unique_ptr<Module> Parser::parse() {
    // Reset state
    position = 0;
    
    try {
        // Parse the module
        parseModule();
        
        // Check for errors
        if (diag.hasErrorDiagnostics()) {
            return nullptr;
        }
        
        return std::move(module);
    } catch (const std::exception& e) {
        // Handle unexpected exceptions
        diag.error(std::string("Parsing failed: ") + e.what(), tokens[position].location);
        return nullptr;
    }
}

Token Parser::peek() const {
    if (isAtEnd()) {
        // Return an EOF token if we're at the end
        return Token(TOKEN_EOF, "", tokens.back().location);
    }
    return tokens[position];
}

Token Parser::previous() const {
    if (position > 0) {
        return tokens[position - 1];
    }
    // Return an error token if there's no previous token
    return Token(TOKEN_ERROR, "", tokens[0].location);
}

Token Parser::advance() {
    if (!isAtEnd()) {
        position++;
    }
    return previous();
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::isAtEnd() const {
    return position >= tokens.size() || tokens[position].type == TOKEN_EOF;
}

void Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        advance();
    } else {
        error(peek(), message);
    }
}

void Parser::error(const std::string& message) {
    diag.error(message, peek().location);
}

void Parser::error(const Token& token, const std::string& message) {
    diag.error(message, token.location);
}

void Parser::parseModule() {
    // Parse top-level declarations
    while (!isAtEnd()) {
        if (match(TOKEN_DIRECTIVE)) {
            if (previous().text == "DIR") {
                parseDirective();
            } else {
                error(previous(), "Expected 'DIR' directive");
                // Skip to the next line
                while (!isAtEnd() && !check(TOKEN_DIRECTIVE)) {
                    advance();
                }
            }
        } else {
            error(peek(), "Expected 'DIR' directive");
            advance(); // Skip the unexpected token
        }
    }
}

void Parser::parseDirective() {
    if (match(TOKEN_IDENTIFIER)) {
        std::string directive = previous().text;
        
        if (directive == "SECT") {
            parseSection();
        } else if (directive == "LABEL") {
            parseLabel();
        } else if (directive == "HINT") {
            parseFunction();
        } else if (directive == "ABI") {
            parseAbi();
        } else {
            error(previous(), "Unknown directive: " + directive);
            // Skip to the next line
            while (!isAtEnd() && !check(TOKEN_DIRECTIVE)) {
                advance();
            }
        }
    } else {
        error(peek(), "Expected directive identifier");
        // Skip to the next line
        while (!isAtEnd() && !check(TOKEN_DIRECTIVE)) {
            advance();
        }
    }
}

void Parser::parseSection() {
    if (match(TOKEN_IDENTIFIER)) {
        std::string sectionName = previous().text;
        
        // Parse section flags
        uint32_t sectionType = SECTION_CODE; // Default to code section
        uint32_t sectionFlags = SECTION_FLAG_ALLOC; // Default to allocatable
        
        while (match(TOKEN_IDENTIFIER)) {
            std::string flag = previous().text;
            
            if (flag == "READ") {
                // All sections are readable
            } else if (flag == "WRITE") {
                sectionFlags |= SECTION_FLAG_WRITE;
            } else if (flag == "EXEC") {
                sectionFlags |= SECTION_FLAG_EXEC;
            } else if (flag == "ALLOC") {
                // Already set by default
            } else if (flag == "NOALLOC") {
                sectionFlags &= ~SECTION_FLAG_ALLOC;
            } else if (flag == "TLS") {
                sectionFlags |= SECTION_FLAG_TLS;
            } else {
                error(previous(), "Unknown section flag: " + flag);
            }
        }
        
        // Determine section type based on name and flags
        if (sectionName == "text" || sectionName == "code") {
            sectionType = SECTION_CODE;
        } else if (sectionName == "data") {
            sectionType = SECTION_DATA;
        } else if (sectionName == "rodata") {
            sectionType = SECTION_READONLY;
        } else if (sectionName == "bss") {
            sectionType = SECTION_BSS;
        }
        
        // Set the current section
        module->setCurrentSection(sectionName, sectionType, sectionFlags);
    } else {
        error(peek(), "Expected section name");
    }
}

void Parser::parseLabel() {
    if (match(TOKEN_IDENTIFIER)) {
        std::string labelName = previous().text;
        
        // TODO: Add the label to the current function or section
        // For now, just log it
        LOG_INFO("Parsed label: " + labelName);
    } else {
        error(peek(), "Expected label name");
    }
}

void Parser::parseFunction() {
    if (match(TOKEN_IDENTIFIER)) {
        std::string functionName = previous().text;
        
        // Parse function attributes
        uint16_t functionFlags = 0;
        
        if (match(TOKEN_IDENTIFIER)) {
            if (previous().text == "FUNC") {
                // Function declaration
                
                // Parse function flags
                while (match(TOKEN_IDENTIFIER)) {
                    std::string flag = previous().text;
                    
                    if (flag == "GLOBAL") {
                        functionFlags |= SYMBOL_FLAG_GLOBAL;
                    } else if (flag == "LOCAL") {
                        functionFlags |= SYMBOL_FLAG_LOCAL;
                    } else if (flag == "WEAK") {
                        functionFlags |= SYMBOL_FLAG_WEAK;
                    } else if (flag == "HIDDEN") {
                        functionFlags |= SYMBOL_FLAG_HIDDEN;
                    } else if (flag == "PROTECTED") {
                        functionFlags |= SYMBOL_FLAG_PROTECTED;
                    } else if (flag == "EXPORTED") {
                        functionFlags |= SYMBOL_FLAG_EXPORTED;
                    } else if (flag == "ENDFUNC") {
                        // End of function (shouldn't be here, but handle it anyway)
                        return;
                    } else {
                        error(previous(), "Unknown function flag: " + flag);
                    }
                }
                
                // Create a new function
                auto function = std::make_unique<Function>(functionName, functionFlags);
                
                // Look for the function body (should start with DIR LABEL)
                if (match(TOKEN_DIRECTIVE) && previous().text == "DIR") {
                    if (match(TOKEN_IDENTIFIER) && previous().text == "LABEL") {
                        if (match(TOKEN_IDENTIFIER) && previous().text == functionName) {
                            // Found the function label, now parse the body
                            
                            // Parse function body (instructions until ENDFUNC)
                            while (!isAtEnd()) {
                                // Check for end of function
                                if (check(TOKEN_DIRECTIVE)) {
                                    Token dir = advance();
                                    if (dir.text == "DIR" && match(TOKEN_IDENTIFIER) && 
                                        previous().text == "HINT" && match(TOKEN_IDENTIFIER) && 
                                        previous().text == functionName && match(TOKEN_IDENTIFIER) && 
                                        previous().text == "ENDFUNC") {
                                        // End of function found
                                        break;
                                    } else {
                                        // Not the end of function, continue parsing
                                        // Go back to start of directive
                                        position -= 3; // DIR HINT functionName
                                    }
                                }
                                
                                // Parse an instruction
                                if (match(TOKEN_INSTRUCTION) || match(TOKEN_IDENTIFIER)) {
                                    // An instruction or directive that we should handle
                                    position--; // Go back to the category/identifier
                                    
                                    // TODO: Implement instruction parsing
                                    // For now, just skip to the next line
                                    while (!isAtEnd() && !check(TOKEN_DIRECTIVE) && !check(TOKEN_INSTRUCTION)) {
                                        advance();
                                    }
                                } else {
                                    // Skip unexpected tokens
                                    advance();
                                }
                            }
                            
                            // Add the function to the module
                            module->addFunction(std::move(function));
                        } else {
                            error(previous(), "Function label doesn't match function name");
                        }
                    } else {
                        error(peek(), "Expected LABEL directive after function declaration");
                    }
                } else {
                    error(peek(), "Expected DIR LABEL after function declaration");
                }
            } else if (previous().text == "ENDFUNC") {
                // End of function
                return;
            } else {
                error(previous(), "Expected FUNC or ENDFUNC");
            }
        } else {
            error(peek(), "Expected FUNC or ENDFUNC");
        }
    } else {
        error(peek(), "Expected function name");
    }
}

void Parser::parseAbi() {
    if (match(TOKEN_IDENTIFIER)) {
        std::string abiName = previous().text;
        
        // Create a new ABI definition
        AbiDefinition abi(abiName);
        
        // Check for opening brace
        if (match(TOKEN_LBRACE)) {
            // Parse ABI definition
            while (!match(TOKEN_RBRACE) && !isAtEnd()) {
                if (match(TOKEN_IDENTIFIER)) {
                    std::string field = previous().text;
                    
                    if (match(TOKEN_EQUALS)) {
                        if (field == "args" && match(TOKEN_LBRACKET)) {
                            // Parse argument registers
                            std::vector<uint8_t> argRegs;
                            
                            while (!match(TOKEN_RBRACKET) && !isAtEnd()) {
                                if (match(TOKEN_REGISTER)) {
                                    argRegs.push_back(previous().regId);
                                } else if (match(TOKEN_COMMA)) {
                                    // Skip commas
                                } else {
                                    error(peek(), "Expected register in argument list");
                                    advance();
                                }
                            }
                            
                            abi.argRegs = argRegs;
                        } else if (field == "rets" && match(TOKEN_LBRACKET)) {
                            // Parse return registers
                            std::vector<uint8_t> retRegs;
                            
                            while (!match(TOKEN_RBRACKET) && !isAtEnd()) {
                                if (match(TOKEN_REGISTER)) {
                                    retRegs.push_back(previous().regId);
                                } else if (match(TOKEN_COMMA)) {
                                    // Skip commas
                                } else {
                                    error(peek(), "Expected register in return list");
                                    advance();
                                }
                            }
                            
                            abi.retRegs = retRegs;
                        } else if (field == "preserved" && match(TOKEN_LBRACKET)) {
                            // Parse preserved registers
                            std::vector<uint8_t> preservedRegs;
                            
                            while (!match(TOKEN_RBRACKET) && !isAtEnd()) {
                                if (match(TOKEN_REGISTER)) {
                                    preservedRegs.push_back(previous().regId);
                                } else if (match(TOKEN_COMMA)) {
                                    // Skip commas
                                } else {
                                    error(peek(), "Expected register in preserved list");
                                    advance();
                                }
                            }
                            
                            abi.preservedRegs = preservedRegs;
                        } else if (field == "volatile" && match(TOKEN_LBRACKET)) {
                            // Parse volatile registers
                            std::vector<uint8_t> volatileRegs;
                            
                            while (!match(TOKEN_RBRACKET) && !isAtEnd()) {
                                if (match(TOKEN_REGISTER)) {
                                    volatileRegs.push_back(previous().regId);
                                } else if (match(TOKEN_COMMA)) {
                                    // Skip commas
                                } else {
                                    error(peek(), "Expected register in volatile list");
                                    advance();
                                }
                            }
                            
                            abi.volatileRegs = volatileRegs;
                        } else if (field == "stack_align") {
                            if (match(TOKEN_INTEGER)) {
                                abi.stackAlign = static_cast<uint32_t>(previous().intValue);
                            } else {
                                error(peek(), "Expected integer for stack alignment");
                            }
                        } else {
                            error(previous(), "Unknown ABI field: " + field);
                            // Skip to the next field
                            while (!isAtEnd() && !check(TOKEN_IDENTIFIER) && !check(TOKEN_RBRACE)) {
                                advance();
                            }
                        }
                    } else {
                        error(peek(), "Expected '=' after ABI field name");
                    }
                } else {
                    error(peek(), "Expected ABI field name");
                    advance();
                }
            }
            
            // Add the ABI definition to the module
            module->addAbiDefinition(abiName, abi);
        } else {
            error(peek(), "Expected '{' after ABI name");
        }
    } else {
        error(peek(), "Expected ABI name");
    }
}

void Parser::parseInstruction() {
    // TODO: Implement instruction parsing
}

std::unique_ptr<Instruction> Parser::parseInstructionBody() {
    // TODO: Implement instruction body parsing
    return nullptr;
}

std::unique_ptr<Operand> Parser::parseOperand() {
    if (match(TOKEN_REGISTER)) {
        return parseRegisterOperand();
    } else if (match(TOKEN_VARIABLE)) {
        return parseVariableOperand();
    } else if (match(TOKEN_INTEGER) || match(TOKEN_FLOAT) || match(TOKEN_STRING)) {
        return parseImmediateOperand();
    } else if (match(TOKEN_LBRACKET)) {
        return parseMemoryOperand();
    } else {
        error(peek(), "Expected operand");
        return nullptr;
    }
}

std::unique_ptr<Operand> Parser::parseRegisterOperand() {
    Token regToken = previous();
    return std::make_unique<RegisterOperand>(REG_GP, regToken.regId);
}

std::unique_ptr<Operand> Parser::parseVariableOperand() {
    Token varToken = previous();
    return std::make_unique<VariableOperand>(varToken.varId);
}

std::unique_ptr<Operand> Parser::parseImmediateOperand() {
    Token immToken = previous();
    
    if (immToken.type == TOKEN_INTEGER) {
        return std::make_unique<ImmediateOperand>(immToken.intValue);
    } else if (immToken.type == TOKEN_FLOAT) {
        return std::make_unique<ImmediateOperand>(immToken.floatValue);
    } else if (immToken.type == TOKEN_STRING) {
        return std::make_unique<ImmediateOperand>(immToken.text);
    } else {
        error(immToken, "Invalid immediate operand");
        return nullptr;
    }
}

std::unique_ptr<Operand> Parser::parseMemoryOperand() {
    // [reg]
    if (match(TOKEN_REGISTER)) {
        Token regToken = previous();
        
        if (match(TOKEN_RBRACKET)) {
            // Simple [reg]
            return std::make_unique<MemoryOperand>(regToken.regId);
        } else if (match(TOKEN_PLUS)) {
            // [reg + ...]
            if (match(TOKEN_REGISTER)) {
                // [reg + reg]
                Token reg2Token = previous();
                
                if (match(TOKEN_STAR)) {
                    // [reg + reg*scale]
                    if (match(TOKEN_INTEGER)) {
                        Token scaleToken = previous();
                        
                        if (match(TOKEN_RBRACKET)) {
                            return std::make_unique<MemoryOperand>(
                                regToken.regId, reg2Token.regId, 
                                static_cast<uint8_t>(scaleToken.intValue));
                        } else {
                            error(peek(), "Expected ']' after memory operand");
                            return nullptr;
                        }
                    } else {
                        error(peek(), "Expected integer scale factor");
                        return nullptr;
                    }
                } else if (match(TOKEN_RBRACKET)) {
                    // [reg + reg]
                    return std::make_unique<MemoryOperand>(regToken.regId, reg2Token.regId);
                } else {
                    error(peek(), "Expected '*' or ']' after register in memory operand");
                    return nullptr;
                }
            } else if (match(TOKEN_INTEGER)) {
                // [reg + disp]
                Token dispToken = previous();
                
                if (match(TOKEN_RBRACKET)) {
                    return std::make_unique<MemoryOperand>(
                        regToken.regId, static_cast<int32_t>(dispToken.intValue));
                } else {
                    error(peek(), "Expected ']' after memory operand");
                    return nullptr;
                }
            } else {
                error(peek(), "Expected register or integer after '+' in memory operand");
                return nullptr;
            }
        } else {
            error(peek(), "Expected ']' or '+' after register in memory operand");
            return nullptr;
        }
    } else {
        error(peek(), "Expected register in memory operand");
        return nullptr;
    }
}

uint16_t Parser::parseTypeSpecifier() {
    if (match(TOKEN_IDENTIFIER)) {
        std::string typeName = previous().text;
        
        // Basic types
        if (typeName == "void") {
            return TYPE_VOID;
        } else if (typeName == "int8") {
            return TYPE_INT8;
        } else if (typeName == "int16") {
            return TYPE_INT16;
        } else if (typeName == "int32") {
            return TYPE_INT32;
        } else if (typeName == "int64") {
            return TYPE_INT64;
        } else if (typeName == "int128") {
            return TYPE_INT128;
        } else if (typeName == "uint8") {
            return TYPE_UINT8;
        } else if (typeName == "uint16") {
            return TYPE_UINT16;
        } else if (typeName == "uint32") {
            return TYPE_UINT32;
        } else if (typeName == "uint64") {
            return TYPE_UINT64;
        } else if (typeName == "uint128") {
            return TYPE_UINT128;
        } else if (typeName == "fp16") {
            return TYPE_FP16;
        } else if (typeName == "fp32") {
            return TYPE_FP32;
        } else if (typeName == "fp64") {
            return TYPE_FP64;
        } else if (typeName == "fp80") {
            return TYPE_FP80;
        } else if (typeName == "fp128") {
            return TYPE_FP128;
        } else if (typeName == "ptr") {
            // Pointer type
            if (match(TOKEN_LPAREN)) {
                uint16_t baseType = parseTypeSpecifier();
                
                if (match(TOKEN_RPAREN)) {
                    return TYPE_PTR | baseType;
                } else {
                    error(peek(), "Expected ')' after pointer base type");
                    return TYPE_PTR;
                }
            } else {
                error(peek(), "Expected '(' after 'ptr'");
                return TYPE_PTR;
            }
        } else if (typeName == "vec128") {
            // 128-bit vector type
            if (match(TOKEN_LPAREN)) {
                uint16_t elemType = parseTypeSpecifier();
                
                if (match(TOKEN_RPAREN)) {
                    return TYPE_VEC128 | elemType;
                } else {
                    error(peek(), "Expected ')' after vector element type");
                    return TYPE_VEC128;
                }
            } else {
                error(peek(), "Expected '(' after 'vec128'");
                return TYPE_VEC128;
            }
        } else if (typeName == "vec256") {
            // 256-bit vector type
            if (match(TOKEN_LPAREN)) {
                uint16_t elemType = parseTypeSpecifier();
                
                if (match(TOKEN_RPAREN)) {
                    return TYPE_VEC256 | elemType;
                } else {
                    error(peek(), "Expected ')' after vector element type");
                    return TYPE_VEC256;
                }
            } else {
                error(peek(), "Expected '(' after 'vec256'");
                return TYPE_VEC256;
            }
        } else if (typeName == "vec512") {
            // 512-bit vector type
            if (match(TOKEN_LPAREN)) {
                uint16_t elemType = parseTypeSpecifier();
                
                if (match(TOKEN_RPAREN)) {
                    return TYPE_VEC512 | elemType;
                } else {
                    error(peek(), "Expected ')' after vector element type");
                    return TYPE_VEC512;
                }
            } else {
                error(peek(), "Expected '(' after 'vec512'");
                return TYPE_VEC512;
            }
        } else {
            error(previous(), "Unknown type name: " + typeName);
            return TYPE_VOID;
        }
    } else {
        error(peek(), "Expected type name");
        return TYPE_VOID;
    }
}

} // namespace coil