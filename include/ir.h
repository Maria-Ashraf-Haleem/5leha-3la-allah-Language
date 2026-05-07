#ifndef IR_H
#define IR_H

#include "parser.h"

typedef enum {
    IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD,
    IR_EQ, IR_NEQ, IR_LT, IR_GT, IR_LEQ, IR_GEQ,
    IR_AND, IR_OR,
    IR_NEG, IR_NOT,
    IR_COPY,
    IR_LABEL, IR_GOTO, IR_IF_FALSE,
    IR_PRINT, IR_ERROR_PRINT, IR_INPUT,
    IR_ARG, IR_CALL, IR_RETURN,
    IR_INC, IR_DEC,
    IR_COMMENT
} IROp;

typedef struct IRInstruction {
    IROp   op;
    char   result[64];
    char   operand1[64];
    char   operand2[64];
    struct IRInstruction *next;
} IRInstruction;

typedef struct {
    IRInstruction *head;
    IRInstruction *tail;
    int            count;
    int            tempCount;
    int            labelCount;
} IRProgram;

IRProgram *createIRProgram(void);
void       freeIRProgram(IRProgram *prog);
IRProgram *generateIR(ASTNode *root);
void       printIR(IRProgram *prog);
int        writeIRToFile(IRProgram *prog, const char *path);

#endif