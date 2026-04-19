#include <stdio.h>
#include "scanner.h"
#include "parser.h"
#include "semantic.h"

int main(void) {
    const char* source =
    "e3mel add(a, b) {\n"
    "    rag3 a + b;\n"
    "}\n"
    "\n"
    "7ot elbdya() {\n"
    "    efda7(x);\n"              // undeclared variable
    "    7ot score = 10;\n"
    "    7ot score = 20;\n"        // duplicate declaration
    "    5las;\n"                  // break outside loop
    "    kammel;\n"                // continue outside loop
    "    efda7(add(5));\n"         // wrong number of arguments
    "    unknownFunc();\n"         // undeclared function
    "}\n";
    printf("=== SCANNER ===\n");
    scan(source);
    printTokens();
    printf("\n=== PARSER ===\n");
    ASTNode *root = parse();

    if (!root) {
        fprintf(stderr, "Parsing failed. Aborting.\n");
        return 1;
    }
    printf("\n=== Abstract Syntax Tree ===\n");
    printAST(root, 0);
    printf("\n=== SEMANTIC ANALYSIS ===\n");
    int semResult = analyzeAST(root);
    freeAST(root);
    return semResult;
}