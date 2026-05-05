#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic2.h"

static int charIndex(char c) {
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= 'A' && c <= 'Z') return 26 + (c - 'A');
    if (c >= '0' && c <= '9') return 52 + (c - '0');
    if (c == '_')              return 62;
    return -1;
}

static TrieNode *newTrieNode(void) {
    TrieNode *n = (TrieNode *)calloc(1, sizeof(TrieNode));
    if (!n) {
        fprintf(stderr, "Out of memory (Trie)\n");
        exit(1);
    }
    return n;
}

static void freeTrieNode(TrieNode *node) {
    if (!node) return;

    for (int i = 0; i < TRIE_ALPHA; i++)
        freeTrieNode(node->children[i]);

    SymbolEntry *e = node->entry;
    while (e) {
        SymbolEntry *next = e->shadowedBy;
        free(e);
        e = next;
    }

    free(node);
}

static TrieNode *trieWalk(TrieNode *root, const char *name, int create) {
    TrieNode *cur = root;

    for (int i = 0; name[i] != '\0'; i++) {
        int idx = charIndex(name[i]);
        if (idx < 0) return NULL;

        if (!cur->children[idx]) {
            if (!create) return NULL;
            cur->children[idx] = newTrieNode();
        }

        cur = cur->children[idx];
    }

    return cur;
}

SymbolTable *createSymbolTable(void) {
    SymbolTable *st = (SymbolTable *)malloc(sizeof(SymbolTable));
    if (!st) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    st->root         = newTrieNode();
    st->topScope     = NULL;
    st->currentLevel = 0;
    return st;
}

void destroySymbolTable(SymbolTable *st) {
    if (!st) return;

    freeTrieNode(st->root);

    ScopeFrame *f = st->topScope;
    while (f) {
        ScopeFrame *prev = f->prev;
        free(f);
        f = prev;
    }

    free(st);
}

void enterScope(SymbolTable *st) {
    ScopeFrame *frame = (ScopeFrame *)calloc(1, sizeof(ScopeFrame));
    if (!frame) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    frame->prev  = st->topScope;
    st->topScope = frame;
    st->currentLevel++;
}

void exitScope(SymbolTable *st) {
    if (!st->topScope) return;

    ScopeFrame *frame = st->topScope;

    for (int i = 0; i < frame->count; i++) {
        TrieNode *node = trieWalk(st->root, frame->names[i], 0);
        if (node && node->entry) {
            SymbolEntry *top = node->entry;
            node->entry      = top->shadowedBy;
            free(top);
        }
    }

    st->topScope = frame->prev;
    free(frame);
    st->currentLevel--;
}

int declareSymbol(SymbolTable *st,
                  const char *name,
                SymbolKind kind,
                int paramCount,
                ValueType type) {
    TrieNode *node = trieWalk(st->root, name, 1);
    if (!node) return 0;

    if (node->entry && node->entry->scopeLevel == st->currentLevel) {
        return 0;
    }

    SymbolEntry *entry = (SymbolEntry *)malloc(sizeof(SymbolEntry));
    if (!entry) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    strncpy(entry->name, name, 99);
    entry->name[99]   = '\0';
    entry->kind       = kind;
    entry->scopeLevel = st->currentLevel;
    entry->paramCount = paramCount;
    entry->type       = type;
    entry->shadowedBy = node->entry;

    node->entry = entry;

    if (st->topScope && st->topScope->count < MAX_DECLS_PER_SCOPE) {
        strncpy(st->topScope->names[st->topScope->count], name, 99);
        st->topScope->names[st->topScope->count][99] = '\0';
        st->topScope->count++;
    }

    return 1;
}

SymbolEntry *lookupSymbol(SymbolTable *st, const char *name) {
    TrieNode *node = trieWalk(st->root, name, 0);
    if (!node) return NULL;
    return node->entry;
}

typedef struct {
    SymbolTable *st;
    int          loopDepth;
    int          inFunction;
    int          errorCount;
} SemCtx;

static void analyzeNode(SemCtx *ctx, ASTNode *node);
static ValueType inferExprType(SemCtx *ctx, ASTNode *node, int reportErrors);

static void semError(SemCtx *ctx, const char *msg) {
    fprintf(stderr, "Semantic error: %s\n", msg);
    ctx->errorCount++;
}

static void analyzeChildren(SemCtx *ctx, ASTNode *node) {
    for (int i = 0; i < node->childCount; i++)
        analyzeNode(ctx, node->children[i]);
}

static const char *typeName(ValueType t) {
    switch (t) {
        case TYPE_NUMBER:  return "number";
        case TYPE_STRING:  return "string";
        case TYPE_BOOL:    return "bool";
        case TYPE_INPUT:   return "input";
        case TYPE_UNKNOWN: return "unknown";
        default:           return "unknown";
    }
}

static int isArithmeticOp(const char *op) {
    return strcmp(op, "+") == 0 ||
        strcmp(op, "-") == 0 ||
        strcmp(op, "*") == 0 ||
        strcmp(op, "/") == 0 ||
        strcmp(op, "%") == 0;
}

static int isComparisonOp(const char *op) {
    return strcmp(op, ">")  == 0 ||
        strcmp(op, "<")  == 0 ||
        strcmp(op, ">=") == 0 ||
        strcmp(op, "<=") == 0;
}

static int isEqualityOp(const char *op) {
    return strcmp(op, "==") == 0 ||
        strcmp(op, "!=") == 0;
}

static int isLogicalOp(const char *op) {
    return strcmp(op, "&&") == 0 ||
        strcmp(op, "||") == 0;
}

static ValueType inferExprType(SemCtx *ctx, ASTNode *node, int reportErrors) {
    if (!node) return TYPE_UNKNOWN;

    char errBuf[256];

    switch (node->kind) {
        case NODE_NUMBER:
            return TYPE_NUMBER;

        case NODE_STRING:
            return TYPE_STRING;

        case NODE_BOOL:
            return TYPE_BOOL;

        case NODE_INPUT:
            return TYPE_INPUT;

        case NODE_IDENTIFIER: {
            SymbolEntry *e = lookupSymbol(ctx->st, node->value);
            if (!e) return TYPE_UNKNOWN;
            return e->type;
        }

        case NODE_GROUP:
            if (node->childCount >= 1)
                return inferExprType(ctx, node->children[0], reportErrors);
            return TYPE_UNKNOWN;

        case NODE_ASSIGN:
            if (node->childCount >= 2)
                return inferExprType(ctx, node->children[1], reportErrors);
            return TYPE_UNKNOWN;

        case NODE_CALL:
            return TYPE_UNKNOWN;

        case NODE_UNARY: {
            if (node->childCount < 1) return TYPE_UNKNOWN;

            ValueType t = inferExprType(ctx, node->children[0], reportErrors);

            if (strcmp(node->value, "!") == 0) {
                if (reportErrors && t != TYPE_UNKNOWN && t != TYPE_BOOL) {
                    snprintf(errBuf, sizeof(errBuf),
                            "operator '!' requires bool but got %s",
                            typeName(t));
                    semError(ctx, errBuf);
                }
                return TYPE_BOOL;
            }

            if (strcmp(node->value, "-") == 0) {
                if (reportErrors && t != TYPE_UNKNOWN && t != TYPE_NUMBER) {
                    snprintf(errBuf, sizeof(errBuf),
                            "unary '-' requires number but got %s",
                            typeName(t));
                    semError(ctx, errBuf);
                }
                return TYPE_NUMBER;
            }

            if (strcmp(node->value, "++") == 0 || strcmp(node->value, "--") == 0) {
                if (reportErrors && t != TYPE_UNKNOWN && t != TYPE_NUMBER) {
                    snprintf(errBuf, sizeof(errBuf),
                            "operator '%s' requires number but got %s",
                            node->value, typeName(t));
                    semError(ctx, errBuf);
                }
                return TYPE_NUMBER;
            }

            return TYPE_UNKNOWN;
        }

        case NODE_POSTFIX: {
            if (node->childCount < 1) return TYPE_UNKNOWN;

            ValueType t = inferExprType(ctx, node->children[0], reportErrors);

            if ((strcmp(node->value, "++") == 0 || strcmp(node->value, "--") == 0) &&
                reportErrors && t != TYPE_UNKNOWN && t != TYPE_NUMBER) {
                snprintf(errBuf, sizeof(errBuf),
                        "operator '%s' requires number but got %s",
                        node->value, typeName(t));
                semError(ctx, errBuf);
            }

            return TYPE_NUMBER;
        }

        case NODE_BINARY: {
            if (node->childCount < 2) return TYPE_UNKNOWN;

            ValueType left  = inferExprType(ctx, node->children[0], reportErrors);
            ValueType right = inferExprType(ctx, node->children[1], reportErrors);

            if (isArithmeticOp(node->value)) {
                if (reportErrors &&
                    left  != TYPE_UNKNOWN && left  != TYPE_NUMBER) {
                    snprintf(errBuf, sizeof(errBuf),
                            "left operand of '%s' must be number but got %s",
                            node->value, typeName(left));
                    semError(ctx, errBuf);
                }
                if (reportErrors &&
                    right != TYPE_UNKNOWN && right != TYPE_NUMBER) {
                    snprintf(errBuf, sizeof(errBuf),
                            "right operand of '%s' must be number but got %s",
                            node->value, typeName(right));
                    semError(ctx, errBuf);
                }
                return TYPE_NUMBER;
            }

            if (isLogicalOp(node->value)) {
                if (reportErrors &&
                    left  != TYPE_UNKNOWN && left  != TYPE_BOOL) {
                    snprintf(errBuf, sizeof(errBuf),
                            "left operand of '%s' must be bool but got %s",
                            node->value, typeName(left));
                    semError(ctx, errBuf);
                }
                if (reportErrors &&
                    right != TYPE_UNKNOWN && right != TYPE_BOOL) {
                    snprintf(errBuf, sizeof(errBuf),
                            "right operand of '%s' must be bool but got %s",
                            node->value, typeName(right));
                    semError(ctx, errBuf);
                }
                return TYPE_BOOL;
            }

            if (isComparisonOp(node->value)) {
                if (reportErrors &&
                    left  != TYPE_UNKNOWN && left  != TYPE_NUMBER) {
                    snprintf(errBuf, sizeof(errBuf),
                            "left operand of '%s' must be number but got %s",
                            node->value, typeName(left));
                    semError(ctx, errBuf);
                }
                if (reportErrors &&
                    right != TYPE_UNKNOWN && right != TYPE_NUMBER) {
                    snprintf(errBuf, sizeof(errBuf),
                            "right operand of '%s' must be number but got %s",
                            node->value, typeName(right));
                    semError(ctx, errBuf);
                }
                return TYPE_BOOL;
            }

            if (isEqualityOp(node->value)) {
                if (reportErrors &&
                    left != TYPE_UNKNOWN &&
                    right != TYPE_UNKNOWN &&
                    left != right) {
                    snprintf(errBuf, sizeof(errBuf),
                            "operands of '%s' must have compatible types (%s vs %s)",
                            node->value, typeName(left), typeName(right));
                    semError(ctx, errBuf);
                }
                return TYPE_BOOL;
            }

            return TYPE_UNKNOWN;
        }

        default:
            return TYPE_UNKNOWN;
    }
}

static void analyzeNode(SemCtx *ctx, ASTNode *node) {
    if (!node) return;

    char errBuf[256];

    switch (node->kind) {

        case NODE_PROGRAM:
            analyzeChildren(ctx, node);
            break;

        case NODE_INCLUDE:
            break;

        case NODE_FUNC_DECL: {
            int paramCount = 0;

            for (int i = 0; i < node->childCount; i++) {
                if (node->children[i]->kind == NODE_IDENTIFIER)
                    paramCount++;
            }

            if (!declareSymbol(ctx->st,
                            node->value,
                            SYM_FUNCTION,
                            paramCount,
                            TYPE_UNKNOWN)) {
                snprintf(errBuf, sizeof(errBuf),
                        "function '%s' already declared in this scope",
                        node->value);
                semError(ctx, errBuf);
            }

            enterScope(ctx->st);

            for (int i = 0; i < node->childCount; i++) {
                if (node->children[i]->kind == NODE_IDENTIFIER) {
                    if (!declareSymbol(ctx->st,
                                    node->children[i]->value,
                                    SYM_PARAMETER,
                                    0,
                                    TYPE_UNKNOWN)) {
                        snprintf(errBuf, sizeof(errBuf),
                                "parameter '%s' already declared in function '%s'",
                                node->children[i]->value, node->value);
                        semError(ctx, errBuf);
                    }
                }
            }

            int savedInFunction = ctx->inFunction;
            ctx->inFunction = 1;

            for (int i = 0; i < node->childCount; i++) {
                if (node->children[i]->kind == NODE_BLOCK)
                    analyzeNode(ctx, node->children[i]);
            }

            ctx->inFunction = savedInFunction;
            exitScope(ctx->st);
            break;
        }

        case NODE_MAIN_DECL: {
            if (!declareSymbol(ctx->st,
                            "elbdya",
                            SYM_FUNCTION,
                            0,
                            TYPE_UNKNOWN)) {
                semError(ctx, "main ('elbdya') declared more than once");
            }

            enterScope(ctx->st);

            int savedInFunction = ctx->inFunction;
            ctx->inFunction = 1;

            analyzeChildren(ctx, node);

            ctx->inFunction = savedInFunction;
            exitScope(ctx->st);
            break;
        }

        case NODE_VAR_DECL: {
            analyzeChildren(ctx, node);

            ValueType initType = TYPE_UNKNOWN;
            if (node->childCount >= 1)
                initType = inferExprType(ctx, node->children[0], 0);

            if (!declareSymbol(ctx->st,
                            node->value,
                            SYM_VARIABLE,
                            0,
                            initType)) {
                snprintf(errBuf, sizeof(errBuf),
                        "variable '%s' already declared in this scope",
                        node->value);
                semError(ctx, errBuf);
            }
            break;
        }

        case NODE_BLOCK:
            enterScope(ctx->st);
            analyzeChildren(ctx, node);
            exitScope(ctx->st);
            break;

        case NODE_IF: {
            if (node->childCount >= 1) {
                analyzeNode(ctx, node->children[0]);

                ValueType condType = inferExprType(ctx, node->children[0], 0);
                if (condType != TYPE_UNKNOWN && condType != TYPE_BOOL) {
                    snprintf(errBuf, sizeof(errBuf),
                            "'law' condition must be bool but got %s",
                            typeName(condType));
                    semError(ctx, errBuf);
                }
            }

            for (int i = 1; i < node->childCount; i++)
                analyzeNode(ctx, node->children[i]);

            break;
        }

        case NODE_WHILE: {
            if (node->childCount >= 1) {
                analyzeNode(ctx, node->children[0]);

                ValueType condType = inferExprType(ctx, node->children[0], 0);
                if (condType != TYPE_UNKNOWN && condType != TYPE_BOOL) {
                    snprintf(errBuf, sizeof(errBuf),
                            "'lflf' condition must be bool but got %s",
                            typeName(condType));
                    semError(ctx, errBuf);
                }
            }

            ctx->loopDepth++;

            if (node->childCount >= 2)
                analyzeNode(ctx, node->children[1]);

            ctx->loopDepth--;
            break;
        }

        case NODE_FOR: {
            enterScope(ctx->st);

            if (node->childCount >= 1)
                analyzeNode(ctx, node->children[0]); /* init */

            if (node->childCount >= 2) {
                analyzeNode(ctx, node->children[1]); /* condition */

                ValueType condType = inferExprType(ctx, node->children[1], 0);
                if (condType != TYPE_UNKNOWN && condType != TYPE_BOOL) {
                    snprintf(errBuf, sizeof(errBuf),
                            "'do5' condition must be bool but got %s",
                            typeName(condType));
                    semError(ctx, errBuf);
                }
            }

            if (node->childCount >= 3)
                analyzeNode(ctx, node->children[2]); /* update */

            ctx->loopDepth++;

            if (node->childCount >= 4)
                analyzeNode(ctx, node->children[3]); /* body */

            ctx->loopDepth--;
            exitScope(ctx->st);
            break;
        }

        case NODE_RETURN:
            if (!ctx->inFunction) {
                semError(ctx, "'rag3' (return) used outside of a function");
            }
            analyzeChildren(ctx, node);
            break;

        case NODE_BREAK:
            if (ctx->loopDepth == 0) {
                semError(ctx, "'5las' (break) used outside of a loop");
            }
            break;

        case NODE_CONTINUE:
            if (ctx->loopDepth == 0) {
                semError(ctx, "'kammel' (continue) used outside of a loop");
            }
            break;

        case NODE_OUTPUT:
        case NODE_ERROR_STMT:
        case NODE_EXPR_STMT:
            analyzeChildren(ctx, node);
            break;

        case NODE_ASSIGN: {
            SymbolEntry *e = NULL;

            if (node->childCount >= 1 &&
                node->children[0]->kind == NODE_IDENTIFIER) {
                e = lookupSymbol(ctx->st, node->children[0]->value);
                if (!e) {
                    snprintf(errBuf, sizeof(errBuf),
                            "variable '%s' used before declaration",
                            node->children[0]->value);
                    semError(ctx, errBuf);
                }
            }

            if (node->childCount >= 2) {
                analyzeNode(ctx, node->children[1]);

                ValueType rhsType = inferExprType(ctx, node->children[1], 0);

                if (e) {
                    if (e->type == TYPE_UNKNOWN && rhsType != TYPE_UNKNOWN) {
                        e->type = rhsType;
                    } else if (e->type != TYPE_UNKNOWN &&
                            rhsType != TYPE_UNKNOWN &&
                            e->type != rhsType) {
                        snprintf(errBuf, sizeof(errBuf),
                                "cannot assign %s to variable '%s' of type %s",
                                typeName(rhsType), e->name, typeName(e->type));
                        semError(ctx, errBuf);
                    }
                }
            }

            break;
        }

        case NODE_IDENTIFIER: {
            SymbolEntry *e = lookupSymbol(ctx->st, node->value);
            if (!e) {
                snprintf(errBuf, sizeof(errBuf),
                        "variable '%s' used before declaration",
                        node->value);
                semError(ctx, errBuf);
            }
            break;
        }

        case NODE_CALL: {
            SymbolEntry *e = lookupSymbol(ctx->st, node->value);

            if (!e) {
                snprintf(errBuf, sizeof(errBuf),
                        "function '%s' is not declared",
                        node->value);
                semError(ctx, errBuf);
            } else if (e->kind != SYM_FUNCTION) {
                snprintf(errBuf, sizeof(errBuf),
                        "'%s' is not a function",
                        node->value);
                semError(ctx, errBuf);
            } else {
                int argCount = node->childCount;
                if (argCount != e->paramCount) {
                    snprintf(errBuf, sizeof(errBuf),
                            "function '%s' expects %d argument(s) but got %d",
                            node->value, e->paramCount, argCount);
                    semError(ctx, errBuf);
                }
            }

            analyzeChildren(ctx, node);
            break;
        }

        case NODE_BINARY:
        case NODE_UNARY:
        case NODE_POSTFIX:
        case NODE_GROUP:
            analyzeChildren(ctx, node);
            (void)inferExprType(ctx, node, 1);
            break;

        case NODE_NUMBER:
        case NODE_STRING:
        case NODE_BOOL:
        case NODE_INPUT:
            break;

        default:
            analyzeChildren(ctx, node);
            break;
    }
}

int analyzeAST(ASTNode *root) {
    if (!root) return 1;

    SymbolTable *st = createSymbolTable();

    SemCtx ctx;
    ctx.st         = st;
    ctx.loopDepth  = 0;
    ctx.inFunction = 0;
    ctx.errorCount = 0;

    enterScope(st);
    analyzeNode(&ctx, root);
    exitScope(st);

    destroySymbolTable(st);

    if (ctx.errorCount == 0) {
        printf("Semantic analysis passed with no errors.\n");
        return 0;
    } else {
        fprintf(stderr, "Semantic analysis finished with %d error(s).\n",
                ctx.errorCount);
        return 1;
    }
}