#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"

extern Token getToken(int index);

static int current  = 0;
static int hadError = 0;

ASTNode *createNode(NodeKind kind, const char *value) {
    ASTNode *n = (ASTNode *)malloc(sizeof(ASTNode));
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    n->kind       = kind;
    n->childCount = 0;
    strncpy(n->value, value ? value : "", 99);
    n->value[99] = '\0';
    return n;
}

void addChild(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) return;
    if (parent->childCount >= AST_MAX_CHILDREN) {
        fprintf(stderr, "AST: too many children\n");
        return;
    }
    parent->children[parent->childCount++] = child;
}

static const char *kindLabel(NodeKind k) {
    switch (k) {
        case NODE_NUMBER:     return "Number";
        case NODE_STRING:     return "String";
        case NODE_BOOL:       return "Bool";
        case NODE_IDENTIFIER: return "Identifier";
        case NODE_UNARY:      return "UnaryOp";
        case NODE_BINARY:     return "BinaryOp";
        case NODE_ASSIGN:     return "Assignment";
        case NODE_CALL:       return "Call";
        case NODE_INPUT:      return "Input(hat)";
        case NODE_GROUP:      return "Group";
        case NODE_POSTFIX:    return "PostfixOp";
        case NODE_PROGRAM:    return "Program";
        case NODE_INCLUDE:    return "Include";
        case NODE_VAR_DECL:   return "VarDecl";
        case NODE_FUNC_DECL:  return "FuncDecl";
        case NODE_MAIN_DECL:  return "MainDecl";
        case NODE_BLOCK:      return "Block";
        case NODE_IF:         return "If";
        case NODE_WHILE:      return "While";
        case NODE_FOR:        return "For";
        case NODE_RETURN:     return "Return";
        case NODE_BREAK:      return "Break";
        case NODE_CONTINUE:   return "Continue";
        case NODE_OUTPUT:     return "Output(efda7)";
        case NODE_ERROR_STMT: return "ErrorStmt(bazet)";
        case NODE_EXPR_STMT:  return "ExprStmt";
        default:              return "Unknown";
    }
}

static void printASTHelper(ASTNode *node, int depth, int isLast, int *prefix) {
    if (!node) return;

    for (int i = 0; i < depth; i++) {
        if (prefix[i])
            printf("|   ");
        else
            printf("    ");
    }

    if (depth > 0) {
        if (isLast)
            printf("`-- ");
        else
            printf("|-- ");
    }

    if (node->value[0])
        printf("%s: %s\n", kindLabel(node->kind), node->value);
    else
        printf("%s\n", kindLabel(node->kind));

    prefix[depth] = !isLast;

    for (int i = 0; i < node->childCount; i++) {
        printASTHelper(node->children[i],
                    depth + 1,
                    i == node->childCount - 1,
                    prefix);
    }
}

void printAST(ASTNode *node, int depth) {
    int prefix[100] = {0};
    printASTHelper(node, 0, 1, prefix);
}

void freeAST(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->childCount; i++)
        freeAST(node->children[i]);
    free(node);
}

Token peek(void)     { return getToken(current); }
Token previous(void) { return getToken(current - 1); }
int   isAtEnd(void)  { return peek().type == TOKEN_EOF; }

Token advance(void) {
    if (!isAtEnd()) current++;
    return previous();
}

int check(TokenType type, KeywordKind kw) {
    if (isAtEnd() && type != TOKEN_EOF) return 0;
    Token t = peek();
    if (t.type != type) return 0;
    if (kw != KW_NONE && t.kwKind != kw) return 0;
    return 1;
}

int match(TokenType type, KeywordKind kw) {
    if (check(type, kw)) { advance(); return 1; }
    return 0;
}

Token consume(TokenType type, KeywordKind kw, const char *msg) {
    if (check(type, kw)) return advance();
    fprintf(stderr, "[Line %d] Syntax error: %s (got '%s')\n",
            peek().line, msg, peek().lexeme);
    hadError = 1;
    return peek();
}

void parseProgram(ASTNode *root) {
    while (!isAtEnd())
        parseDeclaration(root);
}

void parseDeclaration(ASTNode *parent) {
    if (check(TOKEN_KEYWORD, KW_INCLUDE)) {
        parseIncludeDecl(parent);
        return;
    }
    if (check(TOKEN_KEYWORD, KW_VAR)) {
        Token next = getToken(current + 1);
        if (next.type == TOKEN_KEYWORD && next.kwKind == KW_MAIN)
            parseMainDecl(parent);
        else
            parseVarDecl(parent);
        return;
    }
    if (check(TOKEN_KEYWORD, KW_FUNCTION)) {
        parseFunctionDecl(parent);
        return;
    }
    parseStatement(parent);
}

void parseIncludeDecl(ASTNode *parent) {
    consume(TOKEN_KEYWORD,    KW_INCLUDE, "Expected 'geb'");
    Token name = consume(TOKEN_IDENTIFIER, KW_NONE, "Expected library name after 'geb'");
    consume(TOKEN_SEMICOLON,  KW_NONE,    "Expected ';' after include");
    addChild(parent, createNode(NODE_INCLUDE, name.lexeme));
}

void parseMainDecl(ASTNode *parent) {
    consume(TOKEN_KEYWORD, KW_VAR,  "Expected '7ot'");
    consume(TOKEN_KEYWORD, KW_MAIN, "Expected 'elbdya'");
    consume(TOKEN_LPAREN,  KW_NONE, "Expected '(' after 'elbdya'");
    consume(TOKEN_RPAREN,  KW_NONE, "Expected ')' after '('");
    ASTNode *node = createNode(NODE_MAIN_DECL, "elbdya");
    parseBlock(node);
    addChild(parent, node);
}

void parseFunctionDecl(ASTNode *parent) {
    consume(TOKEN_KEYWORD,    KW_FUNCTION, "Expected 'e3mel'");
    Token name = consume(TOKEN_IDENTIFIER, KW_NONE, "Expected function name");
    consume(TOKEN_LPAREN,     KW_NONE,     "Expected '(' after function name");
    ASTNode *node = createNode(NODE_FUNC_DECL, name.lexeme);
    if (!check(TOKEN_RPAREN, KW_NONE))
        parseParameters(node);
    consume(TOKEN_RPAREN, KW_NONE, "Expected ')' after parameters");
    parseBlock(node);
    addChild(parent, node);
}

void parseParameters(ASTNode *parent) {
    Token p = consume(TOKEN_IDENTIFIER, KW_NONE, "Expected parameter name");
    addChild(parent, createNode(NODE_IDENTIFIER, p.lexeme));
    while (match(TOKEN_COMMA, KW_NONE)) {
        p = consume(TOKEN_IDENTIFIER, KW_NONE, "Expected parameter name after ','");
        addChild(parent, createNode(NODE_IDENTIFIER, p.lexeme));
    }
}

void parseBlock(ASTNode *parent) {
    consume(TOKEN_LBRACE, KW_NONE, "Expected '{'");
    ASTNode *block = createNode(NODE_BLOCK, "");
    while (!check(TOKEN_RBRACE, KW_NONE) && !isAtEnd())
        parseDeclaration(block);
    consume(TOKEN_RBRACE, KW_NONE, "Expected '}'");
    addChild(parent, block);
}

void parseVarDecl(ASTNode *parent) {
    consume(TOKEN_KEYWORD,    KW_VAR,  "Expected '7ot'");
    Token name = consume(TOKEN_IDENTIFIER, KW_NONE, "Expected variable name");
    ASTNode *node = createNode(NODE_VAR_DECL, name.lexeme);
    if (match(TOKEN_ASSIGN, KW_NONE))
        addChild(node, parseExpression());
    consume(TOKEN_SEMICOLON, KW_NONE, "Expected ';' after variable declaration");
    addChild(parent, node);
}

void parseStatement(ASTNode *parent) {
    if (check(TOKEN_LBRACE,  KW_NONE))     { parseBlock(parent);        return; }
    if (check(TOKEN_KEYWORD, KW_IF))       { parseIfStmt(parent);       return; }
    if (check(TOKEN_KEYWORD, KW_WHILE))    { parseWhileStmt(parent);    return; }
    if (check(TOKEN_KEYWORD, KW_FOR))      { parseForStmt(parent);      return; }
    if (check(TOKEN_KEYWORD, KW_RETURN))   { parseReturnStmt(parent);   return; }
    if (check(TOKEN_KEYWORD, KW_BREAK))    { parseBreakStmt(parent);    return; }
    if (check(TOKEN_KEYWORD, KW_CONTINUE)) { parseContinueStmt(parent); return; }
    if (check(TOKEN_KEYWORD, KW_OUTPUT))   { parseOutputStmt(parent);   return; }
    if (check(TOKEN_KEYWORD, KW_ERROR))    { parseErrorStmt(parent);    return; }
    parseExprStmt(parent);
}

void parseIfStmt(ASTNode *parent) {
    consume(TOKEN_KEYWORD, KW_IF, "Expected 'law'");
    ASTNode *node = createNode(NODE_IF, "");
    addChild(node, parseExpression());
    parseBlock(node);
    if (match(TOKEN_KEYWORD, KW_ELSE)) {
        if (check(TOKEN_KEYWORD, KW_IF))
            parseIfStmt(node);
        else
            parseBlock(node);
    }
    addChild(parent, node);
}

void parseWhileStmt(ASTNode *parent) {
    consume(TOKEN_KEYWORD, KW_WHILE, "Expected 'lflf'");
    ASTNode *node = createNode(NODE_WHILE, "");
    addChild(node, parseExpression());
    parseBlock(node);
    addChild(parent, node);
}

void parseForStmt(ASTNode *parent) {
    consume(TOKEN_KEYWORD, KW_FOR,  "Expected 'do5'");
    consume(TOKEN_LPAREN,  KW_NONE, "Expected '(' after 'do5'");
    ASTNode *node = createNode(NODE_FOR, "");
    if (!check(TOKEN_SEMICOLON, KW_NONE)) {
        if (check(TOKEN_KEYWORD, KW_VAR)) {
            consume(TOKEN_KEYWORD,    KW_VAR,  "Expected '7ot'");
            Token vname = consume(TOKEN_IDENTIFIER, KW_NONE, "Expected variable name");
            ASTNode *vd = createNode(NODE_VAR_DECL, vname.lexeme);
            if (match(TOKEN_ASSIGN, KW_NONE))
                addChild(vd, parseExpression());
            addChild(node, vd);
        } else {
            addChild(node, parseExpression());
        }
    } else {
        addChild(node, createNode(NODE_EXPR_STMT, "(empty)"));
    }
    consume(TOKEN_SEMICOLON, KW_NONE, "Expected ';' after for-init");
    if (!check(TOKEN_SEMICOLON, KW_NONE))
        addChild(node, parseExpression());
    else
        addChild(node, createNode(NODE_BOOL, "true"));
    consume(TOKEN_SEMICOLON, KW_NONE, "Expected ';' after for-condition");
    if (!check(TOKEN_RPAREN, KW_NONE))
        addChild(node, parseExpression());
    else
        addChild(node, createNode(NODE_EXPR_STMT, "(empty)"));
    consume(TOKEN_RPAREN, KW_NONE, "Expected ')' after for-update");
    parseBlock(node);
    addChild(parent, node);
}

void parseReturnStmt(ASTNode *parent) {
    consume(TOKEN_KEYWORD, KW_RETURN, "Expected 'rag3'");
    ASTNode *node = createNode(NODE_RETURN, "");
    if (!check(TOKEN_SEMICOLON, KW_NONE))
        addChild(node, parseExpression());
    consume(TOKEN_SEMICOLON, KW_NONE, "Expected ';' after return");
    addChild(parent, node);
}

void parseBreakStmt(ASTNode *parent) {
    consume(TOKEN_KEYWORD,   KW_BREAK, "Expected '5las'");
    consume(TOKEN_SEMICOLON, KW_NONE,  "Expected ';' after '5las'");
    addChild(parent, createNode(NODE_BREAK, ""));
}

void parseContinueStmt(ASTNode *parent) {
    consume(TOKEN_KEYWORD,   KW_CONTINUE, "Expected 'kammel'");
    consume(TOKEN_SEMICOLON, KW_NONE,     "Expected ';' after 'kammel'");
    addChild(parent, createNode(NODE_CONTINUE, ""));
}

void parseOutputStmt(ASTNode *parent) {
    consume(TOKEN_KEYWORD, KW_OUTPUT, "Expected 'efda7'");
    consume(TOKEN_LPAREN,  KW_NONE,   "Expected '(' after 'efda7'");
    ASTNode *node = createNode(NODE_OUTPUT, "");
    if (!check(TOKEN_RPAREN, KW_NONE))
        addChild(node, parseExpression());
    consume(TOKEN_RPAREN,    KW_NONE, "Expected ')' after output expression");
    consume(TOKEN_SEMICOLON, KW_NONE, "Expected ';' after output statement");
    addChild(parent, node);
}

void parseErrorStmt(ASTNode *parent) {
    consume(TOKEN_KEYWORD, KW_ERROR, "Expected 'bazet'");
    consume(TOKEN_LPAREN,  KW_NONE,  "Expected '(' after 'bazet'");
    ASTNode *node = createNode(NODE_ERROR_STMT, "");
    if (!check(TOKEN_RPAREN, KW_NONE))
        addChild(node, parseExpression());
    consume(TOKEN_RPAREN,    KW_NONE, "Expected ')' after error expression");
    consume(TOKEN_SEMICOLON, KW_NONE, "Expected ';' after error statement");
    addChild(parent, node);
}

void parseExprStmt(ASTNode *parent) {
    ASTNode *expr = parseExpression();
    consume(TOKEN_SEMICOLON, KW_NONE, "Expected ';' after expression");
    ASTNode *node = createNode(NODE_EXPR_STMT, "");
    addChild(node, expr);
    addChild(parent, node);
}

ASTNode *parseExpression(void) {
    return parseAssignment();
}

ASTNode *parseAssignment(void) {
    if (check(TOKEN_IDENTIFIER, KW_NONE)) {
        Token next = getToken(current + 1);
        if (next.type == TOKEN_ASSIGN) {
            Token name = advance();
            advance();
            ASTNode *node = createNode(NODE_ASSIGN, "=");
            addChild(node, createNode(NODE_IDENTIFIER, name.lexeme));
            addChild(node, parseAssignment());
            return node;
        }
    }
    return parseLogicOr();
}

ASTNode *parseLogicOr(void) {
    ASTNode *left = parseLogicAnd();
    while (match(TOKEN_OR, KW_NONE)) {
        ASTNode *node = createNode(NODE_BINARY, "||");
        addChild(node, left);
        addChild(node, parseLogicAnd());
        left = node;
    }
    return left;
}

ASTNode *parseLogicAnd(void) {
    ASTNode *left = parseEquality();
    while (match(TOKEN_AND, KW_NONE)) {
        ASTNode *node = createNode(NODE_BINARY, "&&");
        addChild(node, left);
        addChild(node, parseEquality());
        left = node;
    }
    return left;
}

ASTNode *parseEquality(void) {
    ASTNode *left = parseComparison();
    while (check(TOKEN_EQUAL, KW_NONE) || check(TOKEN_NOT_EQUAL, KW_NONE)) {
        char op[4];
        strncpy(op, peek().lexeme, 3); op[3] = '\0';
        advance();
        ASTNode *node = createNode(NODE_BINARY, op);
        addChild(node, left);
        addChild(node, parseComparison());
        left = node;
    }
    return left;
}

ASTNode *parseComparison(void) {
    ASTNode *left = parseTerm();
    for (;;) {
        if (check(TOKEN_GREATER_EQUAL, KW_NONE) || check(TOKEN_LESS_EQUAL, KW_NONE)) {
            char op[4];
            strncpy(op, peek().lexeme, 3); op[3] = '\0';
            advance();
            ASTNode *node = createNode(NODE_BINARY, op);
            addChild(node, left);
            addChild(node, parseTerm());
            left = node;
            continue;
        }
        if (check(TOKEN_OPERATOR, KW_NONE)) {
            char lx = peek().lexeme[0];
            if (lx == '>' || lx == '<') {
                char op[2] = { lx, '\0' };
                advance();
                ASTNode *node = createNode(NODE_BINARY, op);
                addChild(node, left);
                addChild(node, parseTerm());
                left = node;
                continue;
            }
        }
        break;
    }
    return left;
}

ASTNode *parseTerm(void) {
    ASTNode *left = parseFactor();
    while (check(TOKEN_OPERATOR, KW_NONE)) {
        char lx = peek().lexeme[0];
        if (lx == '+' || lx == '-') {
            char op[2] = { lx, '\0' };
            advance();
            ASTNode *node = createNode(NODE_BINARY, op);
            addChild(node, left);
            addChild(node, parseFactor());
            left = node;
        } else break;
    }
    return left;
}

ASTNode *parseFactor(void) {
    ASTNode *left = parseUnary();
    while (check(TOKEN_OPERATOR, KW_NONE)) {
        char lx = peek().lexeme[0];
        if (lx == '*' || lx == '/' || lx == '%') {
            char op[2] = { lx, '\0' };
            advance();
            ASTNode *node = createNode(NODE_BINARY, op);
            addChild(node, left);
            addChild(node, parseUnary());
            left = node;
        } else break;
    }
    return left;
}

ASTNode *parseUnary(void) {
    if (match(TOKEN_NOT, KW_NONE)) {
        ASTNode *node = createNode(NODE_UNARY, "!");
        addChild(node, parseUnary());
        return node;
    }
    if (match(TOKEN_INCREMENT, KW_NONE)) {
        ASTNode *node = createNode(NODE_UNARY, "++");
        addChild(node, parseUnary());
        return node;
    }
    if (match(TOKEN_DECREMENT, KW_NONE)) {
        ASTNode *node = createNode(NODE_UNARY, "--");
        addChild(node, parseUnary());
        return node;
    }
    if (check(TOKEN_OPERATOR, KW_NONE) && peek().lexeme[0] == '-') {
        advance();
        ASTNode *node = createNode(NODE_UNARY, "-");
        addChild(node, parseUnary());
        return node;
    }
    return parsePostfix();
}

ASTNode *parsePostfix(void) {
    ASTNode *operand = parsePrimary();
    if (match(TOKEN_INCREMENT, KW_NONE)) {
        ASTNode *node = createNode(NODE_POSTFIX, "++");
        addChild(node, operand);
        return node;
    }
    if (match(TOKEN_DECREMENT, KW_NONE)) {
        ASTNode *node = createNode(NODE_POSTFIX, "--");
        addChild(node, operand);
        return node;
    }
    return operand;
}

ASTNode *parsePrimary(void) {
    if (match(TOKEN_NUMBER, KW_NONE))
        return createNode(NODE_NUMBER, previous().lexeme);

    if (match(TOKEN_STRING, KW_NONE))
        return createNode(NODE_STRING, previous().lexeme);

    if (match(TOKEN_KEYWORD, KW_TRUE))
        return createNode(NODE_BOOL, "true");

    if (match(TOKEN_KEYWORD, KW_FALSE))
        return createNode(NODE_BOOL, "false");

    if (check(TOKEN_KEYWORD, KW_INPUT)) {
        advance();
        consume(TOKEN_LPAREN, KW_NONE, "Expected '(' after 'hat'");
        consume(TOKEN_RPAREN, KW_NONE, "Expected ')' after 'hat('");
        return createNode(NODE_INPUT, "hat");
    }

    if (check(TOKEN_IDENTIFIER, KW_NONE)) {
        Token name = advance();
        if (match(TOKEN_LPAREN, KW_NONE)) {
            ASTNode *node = createNode(NODE_CALL, name.lexeme);
            if (!check(TOKEN_RPAREN, KW_NONE))
                parseArguments(node);
            consume(TOKEN_RPAREN, KW_NONE, "Expected ')' after arguments");
            return node;
        }
        return createNode(NODE_IDENTIFIER, name.lexeme);
    }

    if (match(TOKEN_LPAREN, KW_NONE)) {
        ASTNode *inner = parseExpression();
        consume(TOKEN_RPAREN, KW_NONE, "Expected ')' after expression");
        ASTNode *node = createNode(NODE_GROUP, "");
        addChild(node, inner);
        return node;
    }

    fprintf(stderr, "[Line %d] Syntax error: unexpected token '%s'\n",
            peek().line, peek().lexeme);
    hadError = 1;
    advance();
    return createNode(NODE_NUMBER, "?");
}

void parseArguments(ASTNode *callNode) {
    addChild(callNode, parseExpression());
    while (match(TOKEN_COMMA, KW_NONE))
        addChild(callNode, parseExpression());
}

ASTNode *parse(void) {
    current  = 0;
    hadError = 0;

    ASTNode *root = createNode(NODE_PROGRAM, "");
    parseProgram(root);

    if (hadError) {
        freeAST(root);
        return NULL;
    }

    return root;
}