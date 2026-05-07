#include <stdio.h>
#include "scanner.h"
#include "parser.h"
#include "semantic.h"
#include "ir.h"
#include "codegen.h"
#include "executor.h"

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

    if (semResult != 0) {
        fprintf(stderr, "Semantic analysis failed. IR generation skipped.\n");
        freeAST(root);
        return semResult;
    }

    printf("\n=== INTERMEDIATE CODE ===\n");
    IRProgram *ir = generateIR(root);

    if (!ir) {
        fprintf(stderr, "IR generation failed.\n");
        freeAST(root);
        return 1;
    }

    printIR(ir);

    if (writeIRToFile(ir, "generated/out.ir") == 0) {
        printf("\nIR written to generated/out.ir\n");
    } else {
        fprintf(stderr, "\nFailed to write IR to generated/out.ir\n");
    }

    printf("\n=== CODE GENERATION ===\n");
    if (generateCFromIR(ir, "generated/out.c") == 0) {
        printf("C code written to generated/out.c\n");
    } else {
        fprintf(stderr, "Code generation failed.\n");
        freeIRProgram(ir);
        freeAST(root);
        return 1;
    }

    printf("\n=== EXECUTABLE GENERATION ===\n");
    if (compileGeneratedC("generated/out.c", "generated/program.exe") == 0) {
        printf("Executable written to generated/program.exe\n");
    } else {
        fprintf(stderr, "Executable generation failed.\n");
        freeIRProgram(ir);
        freeAST(root);
        return 1;
    }

    printf("\n=== PROGRAM OUTPUT ===\n");
    if (runExecutable("generated/program.exe", "generated/output.txt") == 0) {
        printFileContent("generated/output.txt");
        printf("\nOutput written to generated/output.txt\n");
    } else {
        fprintf(stderr, "Program execution failed.\n");
        freeIRProgram(ir);
        freeAST(root);
        return 1;
    }

    freeIRProgram(ir);
    freeAST(root);

    return 0;
}
