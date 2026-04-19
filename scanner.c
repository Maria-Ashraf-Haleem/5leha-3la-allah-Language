#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"

static Token tokens[MAX_TOKENS];
static int tokenCount = 0;

static void addToken(TokenType type, KeywordKind kwKind,
                    const char* lexeme, int line) {
    if (tokenCount >= MAX_TOKENS) return;
    tokens[tokenCount].type   = type;
    tokens[tokenCount].kwKind = kwKind;
    tokens[tokenCount].line   = line;
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
        case TOKEN_NOT:            return "NOT_OPERATOR";
        case TOKEN_LESS_EQUAL:     return "LESS_EQUAL_OPERATOR";
        case TOKEN_GREATER_EQUAL:  return "GREATER_EQUAL_OPERATOR";
        case TOKEN_AND:            return "LOGICAL_AND_OPERATOR";
        case TOKEN_OR:             return "LOGICAL_OR_OPERATOR";
        case TOKEN_ASSIGN:         return "ASSIGN_OPERATOR";
        case TOKEN_INCREMENT:      return "INCREMENT_OPERATOR";
        case TOKEN_DECREMENT:      return "DECREMENT_OPERATOR";
        case TOKEN_LPAREN:         return "LPAREN";
        case TOKEN_RPAREN:         return "RPAREN";
        case TOKEN_LBRACE:         return "LBRACE";
        case TOKEN_RBRACE:         return "RBRACE";
        case TOKEN_SEMICOLON:      return "SEMICOLON";
        case TOKEN_COMMA:          return "COMMA";
        default:                   return "EOF";
    }
}

static const char* kwKindName(KeywordKind k) {
    switch (k) {
        case KW_IF:       return "IF";
        case KW_ELSE:     return "ELSE";
        case KW_WHILE:    return "WHILE";
        case KW_FOR:      return "FOR";
        case KW_RETURN:   return "RETURN";
        case KW_BREAK:    return "BREAK";
        case KW_CONTINUE: return "CONTINUE";
        case KW_ERROR:    return "ERROR";
        case KW_INCLUDE:  return "INCLUDE";
        case KW_OUTPUT:   return "OUTPUT";
        case KW_INPUT:    return "INPUT";
        case KW_MAIN:     return "MAIN";
        case KW_FUNCTION: return "FUNCTION";
        case KW_VAR:      return "VAR";
        case KW_TRUE:     return "TRUE";
        case KW_FALSE:    return "FALSE";
        default:          return "UNKNOWN";
    }
}

static KeywordKind getKeywordKind(const char* word) {
    if (strcmp(word, "law")      == 0) return KW_IF;
    if (strcmp(word, "gherKeda") == 0) return KW_ELSE;
    if (strcmp(word, "lflf")     == 0) return KW_WHILE;
    if (strcmp(word, "do5")      == 0) return KW_FOR;
    if (strcmp(word, "rag3")     == 0) return KW_RETURN;
    if (strcmp(word, "5las")     == 0) return KW_BREAK;
    if (strcmp(word, "kammel")   == 0) return KW_CONTINUE;
    if (strcmp(word, "bazet")    == 0) return KW_ERROR;
    if (strcmp(word, "geb")      == 0) return KW_INCLUDE;
    if (strcmp(word, "efda7")    == 0) return KW_OUTPUT;
    if (strcmp(word, "hat")      == 0) return KW_INPUT;
    if (strcmp(word, "elbdya")   == 0) return KW_MAIN;
    if (strcmp(word, "e3mel")    == 0) return KW_FUNCTION;
    if (strcmp(word, "7ot")      == 0) return KW_VAR;
    if (strcmp(word, "true")     == 0) return KW_TRUE;
    if (strcmp(word, "false")    == 0) return KW_FALSE;
    return KW_NONE;
}

static int isIdentStart(int i, const char* source) {
    char c = source[i];
    if (isalpha(c)) return 1;
    if ((c == '5' || c == '3' || c == '7') && isalpha(source[i + 1])) return 1;
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

        if (source[i] == '/' && source[i + 1] == '/') {
            i += 2;
            while (source[i] != '\0' && source[i] != '\n') i++;
            continue;
        }

        if (source[i] == '/' && source[i + 1] == '*') {
            i += 2;
            while (source[i] != '\0' && !(source[i] == '*' && source[i + 1] == '/')) {
                if (source[i] == '\n') line++;
                i++;
            }
            if (source[i] != '\0') i += 2;
            continue;
        }

        if (isIdentStart(i, source)) {
            char buf[100]; int len = 0;
            while (source[i] != '\0' && isIdentPart(source[i])) {
                if (len >= 99) {
                    fprintf(stderr, "Warning: identifier too long at line %d, truncated\n", line);
                    while (source[i] != '\0' && isIdentPart(source[i])) i++;
                    break;
                }
                buf[len++] = source[i++];
            }
            buf[len] = '\0';
            KeywordKind kk = getKeywordKind(buf);
            if (kk != KW_NONE)
                addToken(TOKEN_KEYWORD, kk, buf, line);
            else
                addToken(TOKEN_IDENTIFIER, KW_NONE, buf, line);
            continue;
        }

        if (isdigit(source[i])) {
            char buf[100]; int len = 0;
            while (source[i] != '\0' && isdigit(source[i])) buf[len++] = source[i++];
            if (source[i] == '.' && isdigit(source[i + 1])) {
                buf[len++] = source[i++];
                while (source[i] != '\0' && isdigit(source[i])) buf[len++] = source[i++];
            }
            buf[len] = '\0';
            addToken(TOKEN_NUMBER, KW_NONE, buf, line);
            continue;
        }

        if (source[i] == '"') {
            char buf[100]; int len = 0;
            i++;
            while (source[i] != '\0' && source[i] != '"') {
                if (len >= 99) {
                    fprintf(stderr, "Warning: string too long at line %d, truncated\n", line);
                    while (source[i] != '\0' && source[i] != '"') i++;
                    break;
                }
                if (source[i] == '\\') {
                    char next = source[i + 1];
                    if      (next == '"')  { buf[len++] = '"';  i += 2; }
                    else if (next == 'n')  { buf[len++] = '\n'; i += 2; }
                    else if (next == 't')  { buf[len++] = '\t'; i += 2; }
                    else if (next == '\\') { buf[len++] = '\\'; i += 2; }
                    else                   { buf[len++] = source[i++]; }
                } else {
                    buf[len++] = source[i++];
                }
            }
            buf[len] = '\0';
            if (source[i] == '"') i++;
            addToken(TOKEN_STRING, KW_NONE, buf, line);
            continue;
        }

        if (source[i] == '=' && source[i+1] == '=') { addToken(TOKEN_EQUAL,        KW_NONE, "==", line); i+=2; continue; }
        if (source[i] == '!' && source[i+1] == '=') { addToken(TOKEN_NOT_EQUAL,    KW_NONE, "!=", line); i+=2; continue; }
        if (source[i] == '<' && source[i+1] == '=') { addToken(TOKEN_LESS_EQUAL,   KW_NONE, "<=", line); i+=2; continue; }
        if (source[i] == '>' && source[i+1] == '=') { addToken(TOKEN_GREATER_EQUAL,KW_NONE, ">=", line); i+=2; continue; }
        if (source[i] == '&' && source[i+1] == '&') { addToken(TOKEN_AND,          KW_NONE, "&&", line); i+=2; continue; }
        if (source[i] == '|' && source[i+1] == '|') { addToken(TOKEN_OR,           KW_NONE, "||", line); i+=2; continue; }
        if (source[i] == '+' && source[i+1] == '+') { addToken(TOKEN_INCREMENT,    KW_NONE, "++", line); i+=2; continue; }
        if (source[i] == '-' && source[i+1] == '-') { addToken(TOKEN_DECREMENT,    KW_NONE, "--", line); i+=2; continue; }

        if (source[i] == '=') { addToken(TOKEN_ASSIGN,   KW_NONE, "=", line); i++; continue; }
        if (source[i] == '!') { addToken(TOKEN_NOT,      KW_NONE, "!", line); i++; continue; }
        if (source[i] == '%') { addToken(TOKEN_OPERATOR, KW_NONE, "%", line); i++; continue; }
        if (source[i] == '+' || source[i] == '-' || source[i] == '*' ||
            source[i] == '/' || source[i] == '<' || source[i] == '>') {
            char buf[2] = { source[i], '\0' };
            addToken(TOKEN_OPERATOR, KW_NONE, buf, line);
            i++;
            continue;
        }

        if (source[i] == '(') { addToken(TOKEN_LPAREN,    KW_NONE, "(", line); i++; continue; }
        if (source[i] == ')') { addToken(TOKEN_RPAREN,    KW_NONE, ")", line); i++; continue; }
        if (source[i] == '{') { addToken(TOKEN_LBRACE,    KW_NONE, "{", line); i++; continue; }
        if (source[i] == '}') { addToken(TOKEN_RBRACE,    KW_NONE, "}", line); i++; continue; }
        if (source[i] == ';') { addToken(TOKEN_SEMICOLON, KW_NONE, ";", line); i++; continue; }
        if (source[i] == ',') { addToken(TOKEN_COMMA,     KW_NONE, ",", line); i++; continue; }

        if (source[i] == '[' || source[i] == ']') {
            char buf[2] = { source[i], '\0' };
            addToken(TOKEN_SYMBOL, KW_NONE, buf, line);
            i++;
            continue;
        }

        fprintf(stderr, "Warning: unknown character '%c' at line %d, skipped\n", source[i], line);
        i++;
    }

    addToken(TOKEN_EOF, KW_NONE, "EOF", line);
}

void printTokens(void) {
    printf("%-5s %-24s %s\n", "Line", "Lexeme", "Category");
    printf("--------------------------------------------------\n");
    for (int j = 0; j < tokenCount; j++) {
        if (tokens[j].type == TOKEN_KEYWORD) {
            char cat[64];
            snprintf(cat, sizeof(cat), "KEYWORD(%s)", kwKindName(tokens[j].kwKind));
            printf("%-5d %-24s %s\n", tokens[j].line, tokens[j].lexeme, cat);
        } else {
            printf("%-5d %-24s %s\n",
                tokens[j].line,
                tokens[j].lexeme,
                typeName(tokens[j].type));
        }
    }
}

Token getToken(int index) {
    if (index < 0) index = 0;

    if (index >= tokenCount)
        return tokens[tokenCount - 1];

    return tokens[index];
}

int getTokenCount(void) {
    return tokenCount;
}