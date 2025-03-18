#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include "core/defs.h"
#include "parser/lexer.h"
#include "parser/parser.h"
#include "binary/cof.h"
#include "target/target.h"
#include "util/logger.h"
#include "util/diagnostic.h"

using namespace coil;

/**
 * @brief Print usage information
 * 
 * @param programName Name of the program
 */
void printUsage(const char* programName) {
    std::cout << "COIL Assembler (coilasm) - First generation implementation\n";
    std::cout << "Usage: " << programName << " [options] <input_file>\n";
    std::cout << "Options:\n";
    std::cout << "  -o <output_file>   Specify output file (default: input.cof)\n";
    std::cout << "  -t <target>        Specify target architecture (default: x86-64)\n";
    std::cout << "  -v                 Enable verbose output\n";
    std::cout << "  -h, --help         Display this help message\n";
}

/**
 * @brief Read file contents into a string
 * 
 * @param filename Filename to read
 * @return File contents as a string, or empty string on error
 */
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Could not open file: " << filename << "\n";
        return "";
    }
    
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

/**
 * @brief Main entry point
 * 
 * @param argc Argument count
 * @param argv Argument values
 * @return 0 on success, non-zero on error
 */
int main(int argc, char** argv) {
    // Default options
    std::string inputFile;
    std::string outputFile;
    std::string targetName = "x86-64";
    bool verbose = false;
    
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                outputFile = argv[++i];
            } else {
                std::cerr << "Error: Missing output file after -o\n";
                printUsage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-t") == 0) {
            if (i + 1 < argc) {
                targetName = argv[++i];
            } else {
                std::cerr << "Error: Missing target after -t\n";
                printUsage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (inputFile.empty()) {
            inputFile = argv[i];
        } else {
            std::cerr << "Error: Unexpected argument: " << argv[i] << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Check if input file is specified
    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        printUsage(argv[0]);
        return 1;
    }
    
    // If no output file is specified, use input file with .cof extension
    if (outputFile.empty()) {
        size_t dotPos = inputFile.find_last_of('.');
        if (dotPos != std::string::npos) {
            outputFile = inputFile.substr(0, dotPos) + ".cof";
        } else {
            outputFile = inputFile + ".cof";
        }
    }
    
    // Set up logging
    std::unique_ptr<Logger> logger;
    if (verbose) {
        logger = std::make_unique<ConsoleLogger>(LOG_DEBUG);
    } else {
        logger = std::make_unique<ConsoleLogger>(LOG_INFO);
    }
    GlobalLogger::setInstance(std::move(logger));
    
    // Create diagnostics engine
    DiagnosticEngine diag(GlobalLogger::getInstance());
    
    // Read input file
    std::string sourceCode = readFile(inputFile);
    if (sourceCode.empty()) {
        return 1;
    }
    
    // Process the input file
    LOG_INFO("Processing input file: " + inputFile);
    
    // Tokenize the source code
    Lexer lexer(sourceCode, inputFile, diag);
    std::vector<Token> tokens = lexer.tokenize();
    
    if (diag.hasErrorDiagnostics()) {
        diag.printDiagnostics();
        return 1;
    }
    
    // Parse the tokens
    Parser parser(tokens, diag);
    auto module = parser.parse();
    
    if (diag.hasErrorDiagnostics() || !module) {
        diag.printDiagnostics();
        return 1;
    }
    
    // Generate COF file
    auto cof = module->generateCof();
    if (!cof) {
        LOG_ERROR("Failed to generate COF file");
        return 1;
    }
    
    // Write output file
    if (!cof->write(outputFile)) {
        LOG_ERROR("Failed to write output file: " + outputFile);
        return 1;
    }
    
    LOG_INFO("Successfully wrote output file: " + outputFile);
    
    return 0;
}