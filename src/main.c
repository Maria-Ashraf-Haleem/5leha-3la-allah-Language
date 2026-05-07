#include <stdio.h>
#include <stdlib.h>

#include "scanner.h"
#include "parser.h"
#include "semantic.h"
#include "ir.h"
#include "codegen.h"
#include "executor.h"

static char *readSourceFile(const char *path) {
    FILE *f = fopen(path, "rb");

    if (!f) {
        fprintf(stderr, "Error: cannot open source file: %s\n", path);
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "Error: failed to seek source file: %s\n", path);
        fclose(f);
        return NULL;
    }

    long size = ftell(f);

    if (size < 0) {
        fprintf(stderr, "Error: failed to get source file size: %s\n", path);
        fclose(f);
        return NULL;
    }

    rewind(f);

    char *source = (char *)malloc((size_t)size + 1);

    if (!source) {
        fprintf(stderr, "Error: cannot allocate memory for source file.\n");
        fclose(f);
        return NULL;
    }

    size_t readBytes = fread(source, 1, (size_t)size, f);

    if (readBytes != (size_t)size) {
        fprintf(stderr, "Error: failed to read complete source file: %s\n", path);
        free(source);
        fclose(f);
        return NULL;
    }

    source[size] = '\0';
    fclose(f);

    return source;
}

int main(int argc, char *argv[]) {
    const char *sourcePath = "input/source.txt";

    if (argc >= 2) {
        sourcePath = argv[1];
    }

    char *source = readSourceFile(sourcePath);

    if (!source) {
        return 1;
    }

    printf("=== SOURCE FILE ===\n");
    printf("%s\n", sourcePath);

    printf("\n=== SCANNER ===\n");
    scan(source);
    printTokens();

    printf("\n=== PARSER ===\n");
    ASTNode *root = parse();

    if (!root) {
        fprintf(stderr, "Parsing failed. Aborting.\n");
        free(source);
        return 1;
    }

    printf("\n=== Abstract Syntax Tree ===\n");
    printAST(root, 0);

    printf("\n=== SEMANTIC ANALYSIS ===\n");
    int semResult = analyzeAST(root);

    if (semResult != 0) {
        fprintf(stderr, "Semantic analysis failed. IR generation skipped.\n");
        freeAST(root);
        free(source);
        return semResult;
    }

    printf("\n=== INTERMEDIATE CODE ===\n");
    IRProgram *ir = generateIR(root);

    if (!ir) {
        fprintf(stderr, "IR generation failed.\n");
        freeAST(root);
        free(source);
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
        free(source);
        return 1;
    }

    printf("\n=== EXECUTABLE GENERATION ===\n");

    if (compileGeneratedC("generated/out.c", "generated/program.exe") == 0) {
        printf("Executable written to generated/program.exe\n");
    } else {
        fprintf(stderr, "Executable generation failed.\n");
        freeIRProgram(ir);
        freeAST(root);
        free(source);
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
        free(source);
        return 1;
    }

    freeIRProgram(ir);
    freeAST(root);
    free(source);

    return 0;
}