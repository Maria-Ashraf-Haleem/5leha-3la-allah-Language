#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"

int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("Usage: scanner <code.txt>\n");
        return 1;
    }

    FILE* f = fopen(argv[1], "r");
    if (!f) {
        printf("Error: cannot open file: %s\n", argv[1]);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* source = malloc(size + 1);
    if (!source) {
        printf("Error: cannot allocate memory\n");
        fclose(f);
        return 1;
    }
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);

    scan(source);
    printTokens();

    free(source);
    return 0;
}