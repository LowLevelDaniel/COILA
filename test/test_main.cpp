#include <iostream>
#include <string>
#include <functional>
#include <map>

// Forward declarations of test functions
bool test_lexer();
bool test_parser();
bool test_instruction();
bool test_binary();

int main(int argc, char** argv) {
    // Define test functions
    std::map<std::string, std::function<bool()>> tests = {
        { "lexer", test_lexer },
        { "parser", test_parser },
        { "instruction", test_instruction },
        { "binary", test_binary },
        { "all", []() { 
            return test_lexer() && test_parser() && test_instruction() && test_binary();
        }}
    };
    
    // No arguments, run all tests
    if (argc < 2) {
        std::cout << "Running all tests...\n";
        bool success = tests["all"]();
        return success ? 0 : 1;
    }
    
    // Run specific tests
    for (int i = 1; i < argc; i++) {
        std::string testName = argv[i];
        
        auto it = tests.find(testName);
        if (it != tests.end()) {
            std::cout << "Running " << testName << " tests...\n";
            bool success = it->second();
            
            if (!success) {
                std::cout << "Test " << testName << " failed\n";
                return 1;
            }
            
            std::cout << "Test " << testName << " passed\n";
        } else {
            std::cerr << "Unknown test: " << testName << "\n";
            return 1;
        }
    }
    
    return 0;
}