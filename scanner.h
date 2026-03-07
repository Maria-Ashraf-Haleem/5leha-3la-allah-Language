#ifndef SCANNER_H
#define SCANNER_H

typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_OPERATOR,
    TOKEN_SYMBOL,
    TOKEN_EOF,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_ASSIGN
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[100];
    int line;
} Token;

#define MAX_TOKENS 1024
void scan(const char* source);
void printTokens(void);

#endif