#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"
#include <stdlib.h>

typedef enum { // هنا بنشوف كل انواع العقد اللي ممكن تظهر في الابستراكت سنتاكس تريي
    NODE_NUMBER,
    NODE_STRING,
    NODE_BOOL,
    NODE_IDENTIFIER,
    NODE_UNARY,
    NODE_BINARY,
    NODE_ASSIGN,
    NODE_CALL,
    NODE_INPUT,
    NODE_GROUP,
    NODE_POSTFIX,
    NODE_PROGRAM,
    NODE_INCLUDE,
    NODE_VAR_DECL,
    NODE_FUNC_DECL,
    NODE_MAIN_DECL,
    NODE_BLOCK,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_OUTPUT,
    NODE_ERROR_STMT,
    NODE_EXPR_STMT
} NodeKind;

#define AST_MAX_CHILDREN 64

typedef struct ASTNode {
    NodeKind        kind;
    char            value[100];
    struct ASTNode *children[AST_MAX_CHILDREN]; // كل عقدة ممكن يكون لها لستة من الابناء و كل واحد فيهم ممكن يكون عقدة تانية
    int             childCount;
} ASTNode;

ASTNode *createNode(NodeKind kind, const char *value);
void     addChild(ASTNode *parent, ASTNode *child);
void     printAST(ASTNode *node, int depth);
void     freeAST(ASTNode *node);

ASTNode *parse(void);
Token peek(void); // ترجع التوكن من غير ما تتحرك ع اللي بعده
Token previous(void); // ترجع اخر توكن تم استهلاكها
Token advance(void); // تقرأ التوكن الحالية وتحرك المؤشر للي بعدها
int   check(TokenType type, KeywordKind kw);
int   match(TokenType type, KeywordKind kw);
Token consume(TokenType type, KeywordKind kw, const char *msg);
int   isAtEnd(void);

void     parseProgram(ASTNode *root);
void     parseDeclaration(ASTNode *parent);
void     parseIncludeDecl(ASTNode *parent);
void     parseMainDecl(ASTNode *parent);
void     parseFunctionDecl(ASTNode *parent);
void     parseParameters(ASTNode *parent);
void     parseBlock(ASTNode *parent);
void     parseVarDecl(ASTNode *parent);
void     parseStatement(ASTNode *parent);
void     parseIfStmt(ASTNode *parent);
void     parseWhileStmt(ASTNode *parent);
void     parseForStmt(ASTNode *parent);
void     parseReturnStmt(ASTNode *parent);
void     parseBreakStmt(ASTNode *parent);
void     parseContinueStmt(ASTNode *parent);
void     parseOutputStmt(ASTNode *parent);
void     parseErrorStmt(ASTNode *parent);
void     parseExprStmt(ASTNode *parent);

ASTNode *parseExpression(void);
ASTNode *parseAssignment(void);
ASTNode *parseLogicOr(void);
ASTNode *parseLogicAnd(void);
ASTNode *parseEquality(void);
ASTNode *parseComparison(void);
ASTNode *parseTerm(void);
ASTNode *parseFactor(void);
ASTNode *parseUnary(void);
ASTNode *parsePostfix(void);
ASTNode *parsePrimary(void);
void     parseArguments(ASTNode *callNode); // لما يكون عندنا دالة و بنديها نود من نوع كول، بنضيف لها الابناء اللي هما الارجيومنتس بتاعتها

#endif