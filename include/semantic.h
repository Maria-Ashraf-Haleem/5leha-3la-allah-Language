#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"

typedef enum {
    SYM_VARIABLE,
    SYM_FUNCTION,
    SYM_PARAMETER
} SymbolKind;

typedef enum {
    TYPE_UNKNOWN,
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_INPUT
} ValueType;

typedef struct SymbolEntry {
    char                name[100];
    SymbolKind          kind;
    int                 scopeLevel;
    int                 paramCount;
    ValueType           type;
    struct SymbolEntry *shadowedBy;
} SymbolEntry;

#define TRIE_ALPHA 64

typedef struct TrieNode {
    struct TrieNode *children[TRIE_ALPHA];
    SymbolEntry     *entry;
} TrieNode;

#define MAX_DECLS_PER_SCOPE 256

typedef struct ScopeFrame {
    char              names[MAX_DECLS_PER_SCOPE][100];
    int               count;
    struct ScopeFrame *prev;
} ScopeFrame;

typedef struct {
    TrieNode   *root;
    ScopeFrame *topScope;
    int         currentLevel;
} SymbolTable;

SymbolTable *createSymbolTable(void);
void         destroySymbolTable(SymbolTable *st);

void         enterScope(SymbolTable *st);
void         exitScope(SymbolTable *st);

int          declareSymbol(SymbolTable *st,
                           const char *name,
                        SymbolKind kind,
                        int paramCount,
                        ValueType type);

SymbolEntry *lookupSymbol(SymbolTable *st, const char *name);

int          analyzeAST(ASTNode *root);

#endif