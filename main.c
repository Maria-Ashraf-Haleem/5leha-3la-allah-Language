#include <stdio.h>
#include "scanner.h"
#include "parser.h"
#include "semantic.h"

int main(void) {
    const char* source =
        "// this is a single-line comment\n"
        "/* this is\n"
        "   a multi-line comment */\n"
        "7ot elbdya() {\n"
        "    7ot score = 10 + 5;\n"
        "    7ot pi = 3.14;\n"
        "    law (score == 15 && score != 0) {\n"
        "        efda7(\"hello\");\n"
        "    }\n"
        "    law (score <= 20 || score >= 5) {\n"
        "        efda7(\"he said \\\"hi\\\"\");\n"
        "        rag3 score;\n"
        "    }\n"
        "}";

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