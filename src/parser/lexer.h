#ifndef COIL_PARSER_LEXER_H
#define COIL_PARSER_LEXER_H

#include <string>
#include <vector>
#include <cstdint>
#include "util/source_location.h"
#include "util/diagnostic.h"

namespace coil {

/**
 * @brief Token types
 */
enum TokenType {
    TOKEN_EOF,            // End of file
    TOKEN_IDENTIFIER,     // Identifier
    TOKEN_STRING,         // String literal
    TOKEN_INTEGER,        // Integer literal
    TOKEN_FLOAT,          // Floating-point literal
    TOKEN_REGISTER,       // Register (R0, F0, V0, etc.)
    TOKEN_VARIABLE,       // Variable ($0, $1, etc.)
    TOKEN_COMMA,          // ,
    TOKEN_COLON,          // :
    TOKEN_SEMICOLON,      // ;
    TOKEN_EQUALS,         // =
    TOKEN_LPAREN,         // (
    TOKEN_RPAREN,         // )
    TOKEN_LBRACE,         // {
    TOKEN_RBRACE,         // }
    TOKEN_LBRACKET,       // [
    TOKEN_RBRACKET,       // ]
    TOKEN_PLUS,           // +
    TOKEN_MINUS,          // -
    TOKEN_STAR,           // *
    TOKEN_SLASH,          // /
    TOKEN_PERCENT,        // %
    TOKEN_DOT,            // .
    TOKEN_ARROW,          // ->
    TOKEN_INSTRUCTION,    // Instruction category (CF, MEM, MATH, etc.)
    TOKEN_DIRECTIVE,      // Directive (DIR, etc.)
    TOKEN_LABEL,          // Label identifier
    TOKEN_COMMENT,        // Comment
    TOKEN_ERROR           // Error token
};

/**
 * @brief Token in COIL assembly
 */
struct Token {
    TokenType type;         // Token type
    std::string text;       // Token text
    SourceLocation location; // Token location
    
    // Values for different token types
    union {
        int64_t intValue;    // Integer value
        double floatValue;   // Float value
        uint8_t regId;       // Register ID
        uint8_t varId;       // Variable ID
    };
    
    Token(TokenType t, const std::string& txt, const SourceLocation& loc)
        : type(t), text(txt), location(loc), intValue(0) {}
        
    std::string toString() const;
};

/**
 * @brief Lexer for COIL assembly
 * 
 * Breaks a COIL assembly source into tokens.
 */
class Lexer {
private:
    std::string sourceCode;  // Source code
    std::string filename;    // Source filename
    size_t position;         // Current position in source
    int line;                // Current line number
    int column;              // Current column number
    DiagnosticEngine& diag;  // Diagnostics
    
    // Helper methods
    char peek() const;
    char advance();
    bool match(char expected);
    void skipWhitespace();
    Token scanToken();
    Token scanIdentifier();
    Token scanNumber();
    Token scanString();
    Token scanComment();
    
    bool isAtEnd() const;
    
public:
    /**
     * @brief Construct a new Lexer
     * 
     * @param source Source code
     * @param file Source filename
     * @param diagnostics Diagnostic engine
     */
    Lexer(const std::string& source, const std::string& file, DiagnosticEngine& diagnostics);
    
    /**
     * @brief Tokenize the source code
     * 
     * @return Vector of tokens
     */
    std::vector<Token> tokenize();
    
    /**
     * @brief Get the current source location
     * 
     * @return Source location
     */
    SourceLocation getCurrentLocation() const;
    
    /**
     * @brief Check if an identifier is a category
     * 
     * @param identifier Identifier to check
     * @return true if it's a category, false otherwise
     */
    static bool isCategory(const std::string& identifier);
    
    /**
     * @brief Check if an identifier is a directive
     * 
     * @param identifier Identifier to check
     * @return true if it's a directive, false otherwise
     */
    static bool isDirective(const std::string& identifier);
    
    /**
     * @brief Check if an identifier is a register
     * 
     * @param identifier Identifier to check
     * @param regId Register ID (output parameter)
     * @return true if it's a register, false otherwise
     */
    static bool isRegister(const std::string& identifier, uint8_t& regId);
    
    /**
     * @brief Check if an identifier is a variable
     * 
     * @param identifier Identifier to check
     * @param varId Variable ID (output parameter)
     * @return true if it's a variable, false otherwise
     */
    static bool isVariable(const std::string& identifier, uint8_t& varId);
};

} // namespace coil

#endif // COIL_PARSER_LEXER_H