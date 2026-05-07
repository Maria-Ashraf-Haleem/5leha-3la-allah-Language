#ifndef EXECUTOR_H
#define EXECUTOR_H

int compileGeneratedC(const char *cPath, const char *exePath);
int runExecutable(const char *exePath, const char *outputPath);
void printFileContent(const char *path);

#endif
