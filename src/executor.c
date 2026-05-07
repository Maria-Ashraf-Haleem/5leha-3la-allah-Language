#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/executor.h"

static void toWindowsPath(const char *input, char *output, int outputSize) {
    int j = 0;

    for (int i = 0; input[i] != '\0' && j < outputSize - 1; i++) {
        if (input[i] == '/') {
            output[j++] = '\\';
        } else {
            output[j++] = input[i];
        }
    }

    output[j] = '\0';
}

int compileGeneratedC(const char *cPath, const char *exePath) {
    if (!cPath || !exePath) {
        return -1;
    }

    char command[1024];

    snprintf(command, sizeof(command),
                "gcc %s -o %s",
                cPath, exePath);

    int result = system(command);

    if (result != 0) {
        fprintf(stderr, "Executor: failed to compile generated C code.\n");
        return -1;
    }

    return 0;
}

int runExecutable(const char *exePath, const char *outputPath) {
    if (!exePath || !outputPath) {
        return -1;
    }

    char exeWin[512];
    char outWin[512];
    char command[1024];

#ifdef _WIN32
    toWindowsPath(exePath, exeWin, sizeof(exeWin));
    toWindowsPath(outputPath, outWin, sizeof(outWin));

    if (strncmp(exeWin, ".\\", 2) != 0 && exeWin[0] != '\\') {
        snprintf(command, sizeof(command),
                    ".\\%s > %s 2>&1",
                    exeWin, outWin);
    } else {
        snprintf(command, sizeof(command),
                    "%s > %s 2>&1",
                    exeWin, outWin);
    }
#else
    snprintf(command, sizeof(command),
                "./%s > %s 2>&1",
                exePath, outputPath);
#endif

    int result = system(command);

    if (result != 0) {
        fprintf(stderr, "Executor: failed to run executable.\n");
        return -1;
    }

    return 0;
}

void printFileContent(const char *path) {
    if (!path) {
        return;
    }

    FILE *f = fopen(path, "r");

    if (!f) {
        fprintf(stderr, "Executor: cannot open output file '%s'.\n", path);
        return;
    }

    int c;
    while ((c = fgetc(f)) != EOF) {
        putchar(c);
    }

    fclose(f);
}