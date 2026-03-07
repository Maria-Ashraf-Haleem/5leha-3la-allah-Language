#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"

static Token tokens[MAX_TOKENS];
static int tokenCount = 0;

static void addToken(TokenType type, const char* lexeme, int line) {
    if (tokenCount >= MAX_TOKENS) return;
    tokens[tokenCount].type  = type;
    tokens[tokenCount].line  = line;
    strncpy(tokens[tokenCount].lexeme, lexeme, 99);
    tokens[tokenCount].lexeme[99] = '\0';
    tokenCount++;
}

static const char* typeName(TokenType t) {
    switch (t) {
        case TOKEN_KEYWORD:        return "KEYWORD";
        case TOKEN_IDENTIFIER:     return "IDENTIFIER";
        case TOKEN_NUMBER:         return "NUMBER";
        case TOKEN_STRING:         return "STRING";
        case TOKEN_OPERATOR:       return "OPERATOR";
        case TOKEN_SYMBOL:         return "SYMBOL";
        case TOKEN_EQUAL:          return "EQUALITY_OPERATOR";
        case TOKEN_NOT_EQUAL:      return "NOT_EQUAL_OPERATOR";
        case TOKEN_LESS_EQUAL:     return "LESS_EQUAL_OPERATOR";
        case TOKEN_GREATER_EQUAL:  return "GREATER_EQUAL_OPERATOR";
        case TOKEN_AND:            return "LOGICAL_AND_OPERATOR";
        case TOKEN_OR:             return "LOGICAL_OR_OPERATOR";
        case TOKEN_ASSIGN:         return "ASSIGN_OPERATOR";
        default:                   return "EOF";
    }
}

static int isKeyword(const char* word) {
    const char* keywords[] = {
        "elbdya", "efda7", "hat", "law", "gherKeda",
        "lflf", "do5", "rag3", "5las", "kammel",
        "bazet", "geb", "e3mel", "7ot"
    };
    int count = sizeof(keywords) / sizeof(keywords[0]);
    for (int i = 0; i < count; i++)
        if (strcmp(word, keywords[i]) == 0) return 1;
    return 0;
}

static int isIdentStart(int i, const char* source) {
    char c = source[i];
    if (isalpha(c)) return 1;
    if (c == '5' || c == '3' || c == '7')
        return isalpha(source[i+1]);
    return 0;
}

static int isIdentPart(char c) {
    return isalnum(c) || c == '5' || c == '3' || c == '7';
}

void scan(const char* source) {
    int i = 0;
    int line = 1;
    tokenCount = 0;

    while (source[i] != '\0') {

        if (source[i] == '\n') { line++; i++; continue; }
        if (isspace(source[i])) { i++; continue; }

        if (source[i] == '/' && source[i+1] == '/') {
            while (source[i] && source[i] != '\n') i++;
            continue;
        }

        if (source[i] == '/' && source[i+1] == '*') {
            i += 2;
            while (source[i] && !(source[i] == '*' && source[i+1] == '/')) {
                if (source[i] == '\n') line++;
                i++;
            }
            if (source[i]) i += 2;
            continue;
        }
        if (isIdentStart(i, source)) {
            char buf[100]; int len = 0;
            while (source[i] && isIdentPart(source[i])) {
                if (len >= 99) {
                    fprintf(stderr, "Warning: identifier too long at line %d, truncated\n", line);
                    while (source[i] && isIdentPart(source[i])) i++;
                    break;
                }
                buf[len++] = source[i++];
            }
            buf[len] = '\0';
            addToken(isKeyword(buf) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER, buf, line);
            continue;
        }
        if (isdigit(source[i])) {
            char buf[100]; int len = 0;
            while (source[i] && isdigit(source[i])) buf[len++] = source[i++];
            if (source[i] == '.' && isdigit(source[i+1])) {
                buf[len++] = source[i++];
                while (source[i] && isdigit(source[i])) buf[len++] = source[i++];
            }
            buf[len] = '\0';
            addToken(TOKEN_NUMBER, buf, line);
            continue;
        }

        if (source[i] == '"') {
            char buf[100]; int len = 0;
            i++;
            while (source[i] && source[i] != '"') {
                if (len >= 99) {
                    fprintf(stderr, "Warning: string too long at line %d, truncated\n", line);
                    while (source[i] && source[i] != '"') i++;
                    break;
                }
                if (source[i] == '\\' && source[i+1] == '"') {
                    buf[len++] = '"';
                    i += 2;
                } else if (source[i] == '\\' && source[i+1] == 'n') {
                    buf[len++] = '\n';
                    i += 2;
                } else if (source[i] == '\\' && source[i+1] == '\\') {
                    buf[len++] = '\\';
                    i += 2;
                } else {
                    buf[len++] = source[i++];
                }
            }
            buf[len] = '\0';
            if (source[i]) i++;
            addToken(TOKEN_STRING, buf, line);
            continue;
        }

        if (source[i]=='=' && source[i+1]=='=') { addToken(TOKEN_EQUAL,         "==", line); i+=2; continue; }
        if (source[i]=='!' && source[i+1]=='=') { addToken(TOKEN_NOT_EQUAL,      "!=", line); i+=2; continue; }
        if (source[i]=='<' && source[i+1]=='=') { addToken(TOKEN_LESS_EQUAL,     "<=", line); i+=2; continue; }
        if (source[i]=='>' && source[i+1]=='=') { addToken(TOKEN_GREATER_EQUAL,  ">=", line); i+=2; continue; }
        if (source[i]=='&' && source[i+1]=='&') { addToken(TOKEN_AND,            "&&", line); i+=2; continue; }
        if (source[i]=='|' && source[i+1]=='|') { addToken(TOKEN_OR,             "||", line); i+=2; continue; }
        if (source[i] == '=') { addToken(TOKEN_ASSIGN, "=", line); i++; continue; }
        if (source[i]=='+'||source[i]=='-'||source[i]=='*'||source[i]=='/'||source[i]=='<'||source[i]=='>') {
            char buf[2] = {source[i], '\0'};
            addToken(TOKEN_OPERATOR, buf, line);
            i++;
            continue;
        }

        if (source[i]=='('||source[i]==')'||source[i]=='{'||source[i]=='}'||source[i]==';'||source[i]==',') {
            char buf[2] = {source[i], '\0'};
            addToken(TOKEN_SYMBOL, buf, line);
            i++;
            continue;
        }

        i++;
    }

    addToken(TOKEN_EOF, "EOF", line);
}

void printTokens(void) {
    printf("%-5s %-20s %s\n", "Line", "Lexeme", "Category");
    printf("----------------------------------------\n");
    for (int j = 0; j < tokenCount; j++)
        printf("%-5d %-20s %s\n", tokens[j].line, tokens[j].lexeme, typeName(tokens[j].type));
}