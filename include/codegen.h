#ifndef CODEGEN_H
#define CODEGEN_H

#include "ir.h"

int generateCFromIR(IRProgram *program, const char *outputPath);

#endif