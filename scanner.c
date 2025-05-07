/*
* scanner.c - lexical scanner 
* 
* Implements a lexical scanner for the quantum programming language.
* Converys source text into tokens that can be processed by the parser
*/


#include "quantum_interpreter.h"


/* Initializes scanner with the source code */
void init_scanner(Scanner* scanner, const char* source) {
    scanner->start = source; //points to the start of the current source
    scanner->current = source; // points to the current character
    scanner->line = 1; // Tracks the current line number
}

/* Function to check if character is a digit */
static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

/* Function to check if character is a letter or an underscore */
static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

/* Checks if we're at the end */
static bool is_at_end(Scanner* scanner) {
    return *scanner->current == '\0';
}

/* Advances the scanner to the next character and returns the current one */
static char advance(Scanner* scanner) {
    scanner->current++;
    return scanner->current[-1];
}

/* Returns the current character but doesnt advance */
static char peek(Scanner* scanner) {
    return *scanner->current;
}

/* creates a token of the specified type */
static Token make_token(Scanner* scanner, TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.length = (int)(scanner->current - scanner->start);
    token.line = scanner->line;
    return token;
}

/* Creates an error token with the provided message */
static Token error_token(Scanner* scanner, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner->line;
    return token;
}

/* Skips newlines, whitespaces adn comments */
static void skip_whitespace(Scanner* scanner) {
    for (;;) {
        char c = peek(scanner);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(scanner);
                break;
            case '\n':
                scanner->line++;
                advance(scanner);
                break;
            case '#':
                while (peek(scanner) != '\n' && !is_at_end(scanner)) advance(scanner);
                break;
            default:
                return;
        }
    }
}

/* function to check if current source word matches a keyword */
static TokenType check_keyword(Scanner* scanner, int start, int length,
                             const char* rest, TokenType type) {
    if (scanner->current - scanner->start == start + length &&
        memcmp(scanner->start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_ERROR;
}

/* Determines the token type for an identifier */
static TokenType identifier_type(Scanner* scanner) {
    switch (scanner->start[0]) {
        case 'H': return TOKEN_H;
        case 'X': return TOKEN_X;
        case 'Z': return TOKEN_Z;
        case 'C':
            if (scanner->current - scanner->start > 3 &&
                memcmp(scanner->start, "CNOT", 4) == 0) {
                return TOKEN_CNOT;
            }
            break;
        case 'q':
            return check_keyword(scanner, 1, 4, "ubit", TOKEN_QUBIT);
        case 'm':
            return check_keyword(scanner, 1, 6, "easure", TOKEN_MEASURE);
    }
    return TOKEN_ERROR;
}

/* processes a numeric token */
static Token number(Scanner* scanner) {
    while (is_digit(peek(scanner))) advance(scanner);
    
    Token token = make_token(scanner, TOKEN_NUMBER);
    token.value = strtod(scanner->start, NULL); //converts string to double 
    return token;
}

/* Processes an identifier token */
static Token identifier(Scanner* scanner) {
    while (is_alpha(peek(scanner)) || is_digit(peek(scanner))) advance(scanner);
    
    TokenType type = identifier_type(scanner);
    if (type == TOKEN_ERROR) {
        return error_token(scanner, "Unexpected identifier.");
    }
    return make_token(scanner, type);
}

/* Main function to scan the next token*/
Token scan_token(Scanner* scanner) {
    skip_whitespace(scanner);
    scanner->start = scanner->current;

    if (is_at_end(scanner)) return make_token(scanner, TOKEN_EOF);

    char c = advance(scanner);
    if (is_digit(c)) return number(scanner);
    if (is_alpha(c)) return identifier(scanner);

    switch (c) {
        case '(': return make_token(scanner, TOKEN_LEFT_PAREN);
        case ')': return make_token(scanner, TOKEN_RIGHT_PAREN);
        case ',': return make_token(scanner, TOKEN_COMMA);
        case '\n': return make_token(scanner, TOKEN_NEWLINE);
    }

    return error_token(scanner, "Unexpected character.");
}