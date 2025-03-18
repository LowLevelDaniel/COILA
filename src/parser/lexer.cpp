#include "parser/lexer.h"
#include <sstream>
#include <cctype>
#include <algorithm>
#include <unordered_map>

namespace coil {

// Utility functions
static bool isIdentifierStart(char c) {
    return std::isalpha(c) || c == '_';
}

static bool isIdentifierPart(char c) {
    return std::isalnum(c) || c == '_';
}

// Static maps for categories, directives, and registers
static const std::unordered_map<std::string, bool> categoryMap = {
    {"CF", true},
    {"MEM", true},
    {"MATH", true},
    {"BIT", true},
    {"VEC", true},
    {"ATM", true},
    {"VAR", true},
    {"FRAME", true}
};

static const std::unordered_map<std::string, bool> directiveMap = {
    {"DIR", true},
    {"SECT", true},
    {"LABEL", true},
    {"HINT", true},
    {"FUNC", true},
    {"ENDFUNC", true},
    {"GLOBAL", true},
    {"LOCAL", true},
    {"WEAK", true},
    {"ALIGN", true},
    {"ABI", true},
    {"TARGET", true},
    {"CONFIG", true},
    {"INST", true},
    {"ZERO", true},
    {"ASCII", true},
    {"UNICODE", true},
    {"PADD", true},
    {"INCLUDE", true},
    {"MACRO", true},
    {"ENDM", true},
    {"STRUCT", true},
    {"ENDSTRUCT", true},
    {"CONST", true}
};

// Implementation of Token toString method
std::string Token::toString() const {
    std::ostringstream oss;
    
    switch (type) {
        case TOKEN_EOF:
            oss << "EOF";
            break;
        case TOKEN_IDENTIFIER:
            oss << "IDENTIFIER(" << text << ")";
            break;
        case TOKEN_STRING:
            oss << "STRING(\"" << text << "\")";
            break;
        case TOKEN_INTEGER:
            oss << "INTEGER(" << intValue << ")";
            break;
        case TOKEN_FLOAT:
            oss << "FLOAT(" << floatValue << ")";
            break;
        case TOKEN_REGISTER:
            oss << "REGISTER(" << text << ")";
            break;
        case TOKEN_VARIABLE:
            oss << "VARIABLE(" << text << ")";
            break;
        case TOKEN_INSTRUCTION:
            oss << "INSTRUCTION(" << text << ")";
            break;
        case TOKEN_DIRECTIVE:
            oss << "DIRECTIVE(" << text << ")";
            break;
        case TOKEN_LABEL:
            oss << "LABEL(" << text << ")";
            break;
        default:
            oss << "TOKEN(" << text << ")";
            break;
    }
    
    oss << " at " << location.toString();
    return oss.str();
}

// Lexer implementation
Lexer::Lexer(const std::string& source, const std::string& file, DiagnosticEngine& diagnostics)
    : sourceCode(source), filename(file), position(0), line(1), column(1), diag(diagnostics) {
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        skipWhitespace();
        
        if (isAtEnd()) {
            break;
        }
        
        Token token = scanToken();
        
        if (token.type != TOKEN_COMMENT) {
            tokens.push_back(token);
        }
        
        if (token.type == TOKEN_ERROR) {
            // Continue tokenizing even after an error to catch multiple errors
        }
        
        if (token.type == TOKEN_EOF) {
            break;
        }
    }
    
    return tokens;
}

SourceLocation Lexer::getCurrentLocation() const {
    return SourceLocation(filename, line, column);
}

bool Lexer::isCategory(const std::string& identifier) {
    return categoryMap.find(identifier) != categoryMap.end();
}

bool Lexer::isDirective(const std::string& identifier) {
    return directiveMap.find(identifier) != directiveMap.end();
}

bool Lexer::isRegister(const std::string& identifier, uint8_t& regId) {
    // Check if the string matches a register pattern (R0-R15, F0-F15, V0-V15)
    if (identifier.size() >= 2) {
        char regClass = identifier[0];
        std::string regNumStr = identifier.substr(1);
        
        // Try to parse the register number
        try {
            int regNum = std::stoi(regNumStr);
            
            // Check if it's a valid register ID
            if (regNum >= 0 && regNum <= 15) {
                if (regClass == 'R') {
                    regId = static_cast<uint8_t>(regNum);
                    return true;
                } else if (regClass == 'F') {
                    regId = static_cast<uint8_t>(0x10 + regNum);
                    return true;
                } else if (regClass == 'V') {
                    regId = static_cast<uint8_t>(0x20 + regNum);
                    return true;
                }
            }
        } catch (...) {
            // Not a number
        }
    }
    
    // Check for special registers
    if (identifier == "PC") {
        regId = 0x30;
        return true;
    } else if (identifier == "SP") {
        regId = 0x31;
        return true;
    } else if (identifier == "FP") {
        regId = 0x32;
        return true;
    } else if (identifier == "FLAGS") {
        regId = 0x33;
        return true;
    } else if (identifier == "LR") {
        regId = 0x34;
        return true;
    }
    
    return false;
}

bool Lexer::isVariable(const std::string& identifier, uint8_t& varId) {
    // Check if the string matches a variable pattern ($0-$255)
    if (identifier.size() >= 2 && identifier[0] == '$') {
        std::string varNumStr = identifier.substr(1);
        
        // Try to parse the variable ID
        try {
            int varNum = std::stoi(varNumStr);
            
            // Check if it's a valid variable ID
            if (varNum >= 0 && varNum <= 255) {
                varId = static_cast<uint8_t>(varNum);
                return true;
            }
        } catch (...) {
            // Not a number
        }
    }
    
    return false;
}

// Lexer helper methods
char Lexer::peek() const {
    if (isAtEnd()) {
        return '\0';
    }
    return sourceCode[position];
}

char Lexer::advance() {
    char c = peek();
    position++;
    
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    
    return c;
}

bool Lexer::match(char expected) {
    if (isAtEnd() || peek() != expected) {
        return false;
    }
    
    advance();
    return true;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance();
                break;
            case '\n':
                advance();
                break;
            default:
                return;
        }
    }
}

Token Lexer::scanToken() {
    char c = peek();
    
    if (c == '\0') {
        return Token(TOKEN_EOF, "", getCurrentLocation());
    }
    
    // Check for comments
    if (c == ';') {
        return scanComment();
    }
    
    // Check for identifiers (includes instructions and registers)
    if (isIdentifierStart(c)) {
        return scanIdentifier();
    }
    
    // Check for numbers
    if (std::isdigit(c) || (c == '-' && std::isdigit(peek()))) {
        return scanNumber();
    }
    
    // Check for strings
    if (c == '"') {
        return scanString();
    }
    
    // Other tokens
    advance();
    
    switch (c) {
        case ',': return Token(TOKEN_COMMA, ",", SourceLocation(filename, line, column - 1));
        case ':': return Token(TOKEN_COLON, ":", SourceLocation(filename, line, column - 1));
        case ';': return Token(TOKEN_SEMICOLON, ";", SourceLocation(filename, line, column - 1));
        case '=': return Token(TOKEN_EQUALS, "=", SourceLocation(filename, line, column - 1));
        case '(': return Token(TOKEN_LPAREN, "(", SourceLocation(filename, line, column - 1));
        case ')': return Token(TOKEN_RPAREN, ")", SourceLocation(filename, line, column - 1));
        case '{': return Token(TOKEN_LBRACE, "{", SourceLocation(filename, line, column - 1));
        case '}': return Token(TOKEN_RBRACE, "}", SourceLocation(filename, line, column - 1));
        case '[': return Token(TOKEN_LBRACKET, "[", SourceLocation(filename, line, column - 1));
        case ']': return Token(TOKEN_RBRACKET, "]", SourceLocation(filename, line, column - 1));
        case '+': return Token(TOKEN_PLUS, "+", SourceLocation(filename, line, column - 1));
        case '-': 
            // Check for arrow token
            if (match('>')) {
                return Token(TOKEN_ARROW, "->", SourceLocation(filename, line, column - 2));
            }
            return Token(TOKEN_MINUS, "-", SourceLocation(filename, line, column - 1));
        case '*': return Token(TOKEN_STAR, "*", SourceLocation(filename, line, column - 1));
        case '/': return Token(TOKEN_SLASH, "/", SourceLocation(filename, line, column - 1));
        case '%': return Token(TOKEN_PERCENT, "%", SourceLocation(filename, line, column - 1));
        case '.': return Token(TOKEN_DOT, ".", SourceLocation(filename, line, column - 1));
    }
    
    // Unrecognized character
    SourceLocation errorLoc(filename, line, column - 1);
    std::string errorMsg = "Unexpected character: '" + std::string(1, c) + "'";
    diag.error(errorMsg, errorLoc);
    
    return Token(TOKEN_ERROR, errorMsg, errorLoc);
}

Token Lexer::scanIdentifier() {
    size_t startPos = position;
    int startColumn = column;
    
    while (!isAtEnd() && isIdentifierPart(peek())) {
        advance();
    }
    
    std::string identifier = sourceCode.substr(startPos, position - startPos);
    SourceLocation location(filename, line, startColumn);
    
    // Check if this is a register
    uint8_t regId;
    if (isRegister(identifier, regId)) {
        Token token(TOKEN_REGISTER, identifier, location);
        token.regId = regId;
        return token;
    }
    
    // Check if this is a variable
    uint8_t varId;
    if (isVariable(identifier, varId)) {
        Token token(TOKEN_VARIABLE, identifier, location);
        token.varId = varId;
        return token;
    }
    
    // Check if this is a category or instruction
    if (isCategory(identifier)) {
        return Token(TOKEN_INSTRUCTION, identifier, location);
    }
    
    // Check if this is a directive
    if (isDirective(identifier)) {
        return Token(TOKEN_DIRECTIVE, identifier, location);
    }
    
    // General identifier
    return Token(TOKEN_IDENTIFIER, identifier, location);
}

Token Lexer::scanNumber() {
    size_t startPos = position;
    int startColumn = column;
    bool isNegative = false;
    
    if (peek() == '-') {
        advance(); // Consume the minus sign
        isNegative = true;
    }
    
    while (!isAtEnd() && std::isdigit(peek())) {
        advance();
    }
    
    // Check for fractional part
    bool isFloat = false;
    if (!isAtEnd() && peek() == '.') {
        isFloat = true;
        advance(); // Consume the dot
        
        while (!isAtEnd() && std::isdigit(peek())) {
            advance();
        }
    }
    
    // Check for exponent part
    if (!isAtEnd() && (peek() == 'e' || peek() == 'E')) {
        isFloat = true;
        advance(); // Consume the 'e' or 'E'
        
        if (!isAtEnd() && (peek() == '+' || peek() == '-')) {
            advance(); // Consume the sign
        }
        
        if (!isAtEnd() && std::isdigit(peek())) {
            while (!isAtEnd() && std::isdigit(peek())) {
                advance();
            }
        } else {
            SourceLocation errorLoc(filename, line, column);
            std::string errorMsg = "Expected digits after exponent";
            diag.error(errorMsg, errorLoc);
            return Token(TOKEN_ERROR, errorMsg, errorLoc);
        }
    }
    
    std::string numberText = sourceCode.substr(startPos, position - startPos);
    SourceLocation location(filename, line, startColumn);
    
    if (isFloat) {
        Token token(TOKEN_FLOAT, numberText, location);
        try {
            token.floatValue = std::stod(numberText);
        } catch (...) {
            SourceLocation errorLoc(filename, line, startColumn);
            std::string errorMsg = "Invalid float number: " + numberText;
            diag.error(errorMsg, errorLoc);
            return Token(TOKEN_ERROR, errorMsg, errorLoc);
        }
        return token;
    } else {
        Token token(TOKEN_INTEGER, numberText, location);
        try {
            token.intValue = std::stoll(numberText);
        } catch (...) {
            SourceLocation errorLoc(filename, line, startColumn);
            std::string errorMsg = "Invalid integer number: " + numberText;
            diag.error(errorMsg, errorLoc);
            return Token(TOKEN_ERROR, errorMsg, errorLoc);
        }
        return token;
    }
}

Token Lexer::scanString() {
    size_t startPos = position;
    int startColumn = column;
    
    advance(); // Consume the opening quote
    
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\\') {
            advance(); // Consume the backslash
            
            if (isAtEnd()) {
                SourceLocation errorLoc(filename, line, column);
                std::string errorMsg = "Unterminated escape sequence";
                diag.error(errorMsg, errorLoc);
                return Token(TOKEN_ERROR, errorMsg, errorLoc);
            }
            
            advance(); // Consume the escaped character
        } else {
            advance();
        }
    }
    
    if (isAtEnd()) {
        SourceLocation errorLoc(filename, line, startColumn);
        std::string errorMsg = "Unterminated string";
        diag.error(errorMsg, errorLoc);
        return Token(TOKEN_ERROR, errorMsg, errorLoc);
    }
    
    advance(); // Consume the closing quote
    
    size_t contentStart = startPos + 1;
    size_t contentLength = position - startPos - 2;
    std::string stringContent = sourceCode.substr(contentStart, contentLength);
    SourceLocation location(filename, line, startColumn);
    
    return Token(TOKEN_STRING, stringContent, location);
}

Token Lexer::scanComment() {
    size_t startPos = position;
    int startColumn = column;
    
    advance(); // Consume the semicolon
    
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
    
    std::string commentText = sourceCode.substr(startPos, position - startPos);
    SourceLocation location(filename, line, startColumn);
    
    return Token(TOKEN_COMMENT, commentText, location);
}

bool Lexer::isAtEnd() const {
    return position >= sourceCode.size();
}

} // namespace coil