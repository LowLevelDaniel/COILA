#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "parser/lexer.h"
#include "util/logger.h"
#include "util/diagnostic.h"

using namespace coil;

/**
 * @brief Test that the lexer correctly tokenizes a simple input
 */
bool test_lexer_basic() {
    // Set up logger and diagnostics
    GlobalLogger::setInstance(std::make_unique<ConsoleLogger>(LOG_DEBUG));
    DiagnosticEngine diag(GlobalLogger::getInstance());
    
    // Simple COIL input
    std::string input = "DIR SECT text READ EXEC\n"
                       "DIR LABEL main\n"
                       "  FRAME ENTER\n"
                       "  MEM MOV R0, 42\n"
                       "  FRAME LEAVE\n"
                       "  CF RET";
    
    // Tokenize
    Lexer lexer(input, "test.coil", diag);
    std::vector<Token> tokens = lexer.tokenize();
    
    // Check for errors
    if (diag.hasErrorDiagnostics()) {
        std::cout << "Lexer reported errors:\n";
        diag.printDiagnostics();
        return false;
    }
    
    // Verify token count (excluding EOF)
    if (tokens.size() != 19) {
        std::cout << "Expected 19 tokens, got " << tokens.size() << "\n";
        return false;
    }
    
    // Check first few tokens
    if (tokens[0].type != TOKEN_DIRECTIVE || tokens[0].text != "DIR") {
        std::cout << "Expected first token to be DIR directive\n";
        return false;
    }
    
    if (tokens[1].type != TOKEN_IDENTIFIER || tokens[1].text != "SECT") {
        std::cout << "Expected second token to be SECT identifier\n";
        return false;
    }
    
    if (tokens[2].type != TOKEN_IDENTIFIER || tokens[2].text != "text") {
        std::cout << "Expected third token to be text identifier\n";
        return false;
    }
    
    // Check instruction categories
    if (tokens[10].type != TOKEN_INSTRUCTION || tokens[10].text != "MEM") {
        std::cout << "Expected MEM instruction\n";
        return false;
    }
    
    if (tokens[16].type != TOKEN_INSTRUCTION || tokens[16].text != "CF") {
        std::cout << "Expected CF instruction\n";
        return false;
    }
    
    return true;
}

/**
 * @brief Test that the lexer correctly handles registers and variables
 */
bool test_lexer_registers_variables() {
    // Set up logger and diagnostics
    GlobalLogger::setInstance(std::make_unique<ConsoleLogger>(LOG_DEBUG));
    DiagnosticEngine diag(GlobalLogger::getInstance());
    
    // Input with registers and variables
    std::string input = "MEM MOV R0, $0\n"
                       "MEM MOV F1, $10\n"
                       "VEC ADD V2, V3, [R4 + R5]";
    
    // Tokenize
    Lexer lexer(input, "test.coil", diag);
    std::vector<Token> tokens = lexer.tokenize();
    
    // Check for errors
    if (diag.hasErrorDiagnostics()) {
        std::cout << "Lexer reported errors:\n";
        diag.printDiagnostics();
        return false;
    }
    
    // Check register tokens
    if (tokens[2].type != TOKEN_REGISTER || tokens[2].text != "R0" || tokens[2].regId != 0) {
        std::cout << "Expected R0 register\n";
        return false;
    }
    
    if (tokens[8].type != TOKEN_REGISTER || tokens[8].text != "F1" || tokens[8].regId != 0x11) {
        std::cout << "Expected F1 register\n";
        return false;
    }
    
    if (tokens[14].type != TOKEN_REGISTER || tokens[14].text != "V2" || tokens[14].regId != 0x22) {
        std::cout << "Expected V2 register\n";
        return false;
    }
    
    // Check variable tokens
    if (tokens[4].type != TOKEN_VARIABLE || tokens[4].text != "$0" || tokens[4].varId != 0) {
        std::cout << "Expected $0 variable\n";
        return false;
    }
    
    if (tokens[10].type != TOKEN_VARIABLE || tokens[10].text != "$10" || tokens[10].varId != 10) {
        std::cout << "Expected $10 variable\n";
        return false;
    }
    
    // Check memory operand tokens
    if (tokens[18].type != TOKEN_LBRACKET) {
        std::cout << "Expected [\n";
        return false;
    }
    
    if (tokens[19].type != TOKEN_REGISTER || tokens[19].text != "R4") {
        std::cout << "Expected R4 register\n";
        return false;
    }
    
    return true;
}

/**
 * @brief Test that the lexer correctly handles strings and numbers
 */
bool test_lexer_literals() {
    // Set up logger and diagnostics
    GlobalLogger::setInstance(std::make_unique<ConsoleLogger>(LOG_DEBUG));
    DiagnosticEngine diag(GlobalLogger::getInstance());
    
    // Input with literals
    std::string input = "MEM MOV R0, 42\n"
                       "MEM MOV R1, -100\n"
                       "MEM MOV F0, 3.14159\n"
                       "DIR INST \"Hello, World!\"";
    
    // Tokenize
    Lexer lexer(input, "test.coil", diag);
    std::vector<Token> tokens = lexer.tokenize();
    
    // Check for errors
    if (diag.hasErrorDiagnostics()) {
        std::cout << "Lexer reported errors:\n";
        diag.printDiagnostics();
        return false;
    }
    
    // Check integer literals
    if (tokens[4].type != TOKEN_INTEGER || tokens[4].intValue != 42) {
        std::cout << "Expected integer 42\n";
        return false;
    }
    
    if (tokens[10].type != TOKEN_INTEGER || tokens[10].intValue != -100) {
        std::cout << "Expected integer -100\n";
        return false;
    }
    
    // Check float literal
    if (tokens[16].type != TOKEN_FLOAT || std::abs(tokens[16].floatValue - 3.14159) > 1e-5) {
        std::cout << "Expected float 3.14159\n";
        return false;
    }
    
    // Check string literal
    if (tokens[20].type != TOKEN_STRING || tokens[20].text != "Hello, World!") {
        std::cout << "Expected string \"Hello, World!\"\n";
        return false;
    }
    
    return true;
}

/**
 * @brief Run all lexer tests
 */
bool test_lexer() {
    std::cout << "Testing lexer...\n";
    
    bool success = true;
    
    success &= test_lexer_basic();
    success &= test_lexer_registers_variables();
    success &= test_lexer_literals();
    
    if (success) {
        std::cout << "All lexer tests passed.\n";
    } else {
        std::cout << "Some lexer tests failed.\n";
    }
    
    return success;
}