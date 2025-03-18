#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "parser/lexer.h"
#include "parser/parser.h"
#include "util/logger.h"
#include "util/diagnostic.h"

using namespace coil;

/**
 * @brief Test that the parser correctly parses a simple module
 */
bool test_parser_basic() {
    // Set up logger and diagnostics
    GlobalLogger::setInstance(std::make_unique<ConsoleLogger>(LOG_DEBUG));
    DiagnosticEngine diag(GlobalLogger::getInstance());
    
    // Simple COIL input
    std::string input = "DIR SECT text READ EXEC\n"
                        "DIR HINT main FUNC GLOBAL\n"
                        "DIR LABEL main\n"
                        "  FRAME ENTER\n"
                        "  MEM MOV R0, 42\n"
                        "  FRAME LEAVE\n"
                        "  CF RET\n"
                        "DIR HINT main ENDFUNC";
    
    // Tokenize
    Lexer lexer(input, "test.coil", diag);
    std::vector<Token> tokens = lexer.tokenize();
    
    // Check for lexer errors
    if (diag.hasErrorDiagnostics()) {
        std::cout << "Lexer reported errors:\n";
        diag.printDiagnostics();
        return false;
    }
    
    // Parse the tokens
    Parser parser(tokens, diag);
    auto module = parser.parse();
    
    // Check for parser errors
    if (diag.hasErrorDiagnostics()) {
        std::cout << "Parser reported errors:\n";
        diag.printDiagnostics();
        return false;
    }
    
    // Verify that the module was created
    if (!module) {
        std::cout << "Expected non-null module\n";
        return false;
    }
    
    // Verify that the function was parsed
    auto func = module->getFunctionByName("main");
    if (!func) {
        std::cout << "Expected module to contain function 'main'\n";
        return false;
    }
    
    // Verify that the function has the correct flags
    if ((func->getFlags() & SYMBOL_FLAG_GLOBAL) == 0) {
        std::cout << "Expected function 'main' to have GLOBAL flag\n";
        return false;
    }
    
    return true;
}

/**
 * @brief Test that the parser correctly parses an ABI definition
 */
bool test_parser_abi() {
    // Set up logger and diagnostics
    GlobalLogger::setInstance(std::make_unique<ConsoleLogger>(LOG_DEBUG));
    DiagnosticEngine diag(GlobalLogger::getInstance());
    
    // COIL input with ABI definition
    std::string input = "DIR ABI abi-linux-x86_64\n"
                        "{\n"
                        "  args = [ R0, R4, R5, R3, R6, R7 ]\n"
                        "  rets = [ R0 ]\n"
                        "  preserved = [ R1, R10, R11, R12, R13 ]\n"
                        "  volatile = [ R0, R2, R3, R4, R5, R6, R7, R8, R9 ]\n"
                        "  stack_align = 16\n"
                        "}";
    
    // Tokenize
    Lexer lexer(input, "test.coil", diag);
    std::vector<Token> tokens = lexer.tokenize();
    
    // Check for lexer errors
    if (diag.hasErrorDiagnostics()) {
        std::cout << "Lexer reported errors:\n";
        diag.printDiagnostics();
        return false;
    }
    
    // Parse the tokens
    Parser parser(tokens, diag);
    auto module = parser.parse();
    
    // Check for parser errors
    if (diag.hasErrorDiagnostics()) {
        std::cout << "Parser reported errors:\n";
        diag.printDiagnostics();
        return false;
    }
    
    // Verify that the module was created
    if (!module) {
        std::cout << "Expected non-null module\n";
        return false;
    }
    
    // Verify that the ABI was parsed
    const auto* abi = module->getAbiDefinition("abi-linux-x86_64");
    if (!abi) {
        std::cout << "Expected module to contain ABI 'abi-linux-x86_64'\n";
        return false;
    }
    
    // Verify ABI properties
    if (abi->argRegs.size() != 6) {
        std::cout << "Expected ABI to have 6 argument registers, got " << abi->argRegs.size() << "\n";
        return false;
    }
    
    if (abi->retRegs.size() != 1 || abi->retRegs[0] != REG_R0) {
        std::cout << "Expected ABI to have R0 as return register\n";
        return false;
    }
    
    if (abi->stackAlign != 16) {
        std::cout << "Expected ABI to have stack alignment of 16, got " << abi->stackAlign << "\n";
        return false;
    }
    
    return true;
}

/**
 * @brief Test that the parser correctly parses multiple sections
 */
bool test_parser_sections() {
    // Set up logger and diagnostics
    GlobalLogger::setInstance(std::make_unique<ConsoleLogger>(LOG_DEBUG));
    DiagnosticEngine diag(GlobalLogger::getInstance());
    
    // COIL input with multiple sections
    std::string input = "DIR SECT text READ EXEC\n"
                        "DIR HINT func1 FUNC GLOBAL\n"
                        "DIR LABEL func1\n"
                        "  FRAME ENTER\n"
                        "  FRAME LEAVE\n"
                        "  CF RET\n"
                        "DIR HINT func1 ENDFUNC\n"
                        "\n"
                        "DIR SECT data READ\n"
                        "DIR LABEL data1\n"
                        "  DIR INST \"Hello, World!\"\n"
                        "\n"
                        "DIR SECT bss READ WRITE\n"
                        "DIR LABEL bss1\n"
                        "  DIR ZERO 1024\n";
    
    // Tokenize
    Lexer lexer(input, "test.coil", diag);
    std::vector<Token> tokens = lexer.tokenize();
    
    // Check for lexer errors
    if (diag.hasErrorDiagnostics()) {
        std::cout << "Lexer reported errors:\n";
        diag.printDiagnostics();
        return false;
    }
    
    // Parse the tokens
    Parser parser(tokens, diag);
    auto module = parser.parse();
    
    // Check for parser errors
    if (diag.hasErrorDiagnostics()) {
        std::cout << "Parser reported errors:\n";
        diag.printDiagnostics();
        return false;
    }
    
    // Verify that the module was created
    if (!module) {
        std::cout << "Expected non-null module\n";
        return false;
    }
    
    // Verify that the function was parsed
    auto func = module->getFunctionByName("func1");
    if (!func) {
        std::cout << "Expected module to contain function 'func1'\n";
        return false;
    }
    
    // Verify the current section
    if (module->getCurrentSection() != "bss") {
        std::cout << "Expected current section to be 'bss', got '" << module->getCurrentSection() << "'\n";
        return false;
    }
    
    // Verify section flags
    if ((module->getCurrentSectionFlags() & SECTION_FLAG_WRITE) == 0) {
        std::cout << "Expected 'bss' section to have WRITE flag\n";
        return false;
    }
    
    return true;
}

/**
 * @brief Run all parser tests
 */
bool test_parser() {
    std::cout << "Testing parser...\n";
    
    bool success = true;
    
    success &= test_parser_basic();
    success &= test_parser_abi();
    success &= test_parser_sections();
    
    if (success) {
        std::cout << "All parser tests passed.\n";
    } else {
        std::cout << "Some parser tests failed.\n";
    }
    
    return success;
}