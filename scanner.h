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
    TOKEN_NOT,
    TOKEN_LESS_EQUAL, 
    TOKEN_GREATER_EQUAL,
    TOKEN_AND, TOKEN_OR,
    TOKEN_ASSIGN,
    TOKEN_INCREMENT, 
    TOKEN_DECREMENT,
    TOKEN_LPAREN, 
    TOKEN_RPAREN,
    TOKEN_LBRACE, 
    TOKEN_RBRACE,
    TOKEN_SEMICOLON, 
    TOKEN_COMMA
} TokenType;

typedef enum {
    KW_NONE, 
    KW_IF, 
    KW_ELSE, 
    KW_WHILE, 
    KW_FOR,
    KW_RETURN, 
    KW_BREAK, 
    KW_CONTINUE, 
    KW_ERROR,
    KW_INCLUDE, 
    KW_OUTPUT, 
    KW_INPUT, 
    KW_MAIN,
    KW_FUNCTION, 
    KW_VAR, 
    KW_TRUE, 
    KW_FALSE
} KeywordKind;

typedef struct {
    TokenType   type;
    KeywordKind kwKind;
    char        lexeme[100];
    int         line;
} Token;

#define MAX_TOKENS 1024

void scan(const char* source);
void printTokens(void);
Token getToken(int index);
int getTokenCount(void);

#endif