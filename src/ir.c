#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"

#define LOOP_STACK_MAX 64

typedef struct {
    char startLabel[64];
    char endLabel[64];
} LoopFrame;

static LoopFrame loopStack[LOOP_STACK_MAX];
static int       loopTop = -1;

static void pushLoop(const char *start, const char *end) {
    if (loopTop + 1 >= LOOP_STACK_MAX) {
        fprintf(stderr, "IR: loop nesting too deep\n");
        return;
    }
    loopTop++;
    strncpy(loopStack[loopTop].startLabel, start, 63);
    strncpy(loopStack[loopTop].endLabel,   end,   63);
}

static void popLoop(void) {
    if (loopTop >= 0) loopTop--;
}

IRProgram *createIRProgram(void) {
    IRProgram *p = (IRProgram *)malloc(sizeof(IRProgram));
    if (!p) { fprintf(stderr, "IR: out of memory\n"); exit(1); }
    p->head       = NULL;
    p->tail       = NULL;
    p->count      = 0;
    p->tempCount  = 0;
    p->labelCount = 0;
    return p;
}

void freeIRProgram(IRProgram *prog) {
    if (!prog) return;
    IRInstruction *cur = prog->head;
    while (cur) {
        IRInstruction *nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    free(prog);
}

static IRInstruction *emit(IRProgram *prog, IROp op,
                           const char *result, const char *op1, const char *op2) {
    IRInstruction *ins = (IRInstruction *)malloc(sizeof(IRInstruction));
    if (!ins) { fprintf(stderr, "IR: out of memory\n"); exit(1); }
    ins->op   = op;
    ins->next = NULL;
    strncpy(ins->result,   result ? result : "", 63); ins->result[63]   = '\0';
    strncpy(ins->operand1, op1    ? op1    : "", 63); ins->operand1[63] = '\0';
    strncpy(ins->operand2, op2    ? op2    : "", 63); ins->operand2[63] = '\0';
    if (!prog->tail) {
        prog->head = prog->tail = ins;
    } else {
        prog->tail->next = ins;
        prog->tail       = ins;
    }
    prog->count++;
    return ins;
}

static char *newTemp(IRProgram *prog) {
    prog->tempCount++;
    char *buf = (char *)malloc(16);
    if (!buf) { fprintf(stderr, "IR: out of memory\n"); exit(1); }
    snprintf(buf, 16, "t%d", prog->tempCount);
    return buf;
}

static char *newLabel(IRProgram *prog) {
    prog->labelCount++;
    char *buf = (char *)malloc(16);
    if (!buf) { fprintf(stderr, "IR: out of memory\n"); exit(1); }
    snprintf(buf, 16, "L%d", prog->labelCount);
    return buf;
}

static void generateIRStmt(IRProgram *prog, ASTNode *node);

static char *generateIRExpr(IRProgram *prog, ASTNode *node) {
    if (!node) {
        char *r = malloc(8); strcpy(r, "?"); return r;
    }

    if (node->kind == NODE_NUMBER || node->kind == NODE_BOOL) {
        char *r = malloc(64);
        strncpy(r, node->value, 63); r[63] = '\0';
        return r;
    }

    if (node->kind == NODE_STRING) {
        char *r = malloc(128);
        snprintf(r, 128, "\"%s\"", node->value);
        return r;
    }

    if (node->kind == NODE_IDENTIFIER) {
        char *r = malloc(64);
        strncpy(r, node->value, 63); r[63] = '\0';
        return r;
    }

    if (node->kind == NODE_GROUP) {
        return generateIRExpr(prog, node->children[0]);
    }

    if (node->kind == NODE_INPUT) {
        char *tmp = newTemp(prog);
        emit(prog, IR_INPUT, tmp, "", "");
        return tmp;
    }

    if (node->kind == NODE_BINARY) {
        char *left  = generateIRExpr(prog, node->children[0]);
        char *right = generateIRExpr(prog, node->children[1]);
        char *tmp   = newTemp(prog);
        IROp op;
        const char *v = node->value;
        if      (strcmp(v, "+")  == 0) op = IR_ADD;
        else if (strcmp(v, "-")  == 0) op = IR_SUB;
        else if (strcmp(v, "*")  == 0) op = IR_MUL;
        else if (strcmp(v, "/")  == 0) op = IR_DIV;
        else if (strcmp(v, "%")  == 0) op = IR_MOD;
        else if (strcmp(v, "==") == 0) op = IR_EQ;
        else if (strcmp(v, "!=") == 0) op = IR_NEQ;
        else if (strcmp(v, "<")  == 0) op = IR_LT;
        else if (strcmp(v, ">")  == 0) op = IR_GT;
        else if (strcmp(v, "<=") == 0) op = IR_LEQ;
        else if (strcmp(v, ">=") == 0) op = IR_GEQ;
        else if (strcmp(v, "&&") == 0) op = IR_AND;
        else if (strcmp(v, "||") == 0) op = IR_OR;
        else {
            char comment[256];
            snprintf(comment, sizeof(comment), "unsupported binary op '%s'", v);
            emit(prog, IR_COMMENT, "", comment, "");
            op = IR_ADD;
        }
        emit(prog, op, tmp, left, right);
        free(left);
        free(right);
        return tmp;
    }

    if (node->kind == NODE_UNARY) {
        const char *v = node->value;
        if (strcmp(v, "++") == 0 || strcmp(v, "--") == 0) {
            char *inner = generateIRExpr(prog, node->children[0]);
            char *tmp   = newTemp(prog);
            IROp  op    = (strcmp(v, "++") == 0) ? IR_ADD : IR_SUB;
            emit(prog, op, tmp, inner, "1");
            emit(prog, IR_COPY, inner, tmp, "");
            free(inner);
            return tmp;
        }
        char *operand = generateIRExpr(prog, node->children[0]);
        char *tmp     = newTemp(prog);
        IROp  op      = (strcmp(v, "!") == 0) ? IR_NOT : IR_NEG;
        emit(prog, op, tmp, operand, "");
        free(operand);
        return tmp;
    }

    if (node->kind == NODE_POSTFIX) {
        char *inner  = generateIRExpr(prog, node->children[0]);
        char *oldVal = newTemp(prog);
        emit(prog, IR_COPY, oldVal, inner, "");
        char *newVal = newTemp(prog);
        IROp  op = (strcmp(node->value, "++") == 0) ? IR_ADD : IR_SUB;
        emit(prog, op, newVal, inner, "1");
        emit(prog, IR_COPY, inner, newVal, "");
        free(inner);
        free(newVal);
        return oldVal;
    }

    if (node->kind == NODE_ASSIGN) {
        char *target = node->children[0]->value;
        char *rhs    = generateIRExpr(prog, node->children[1]);
        emit(prog, IR_COPY, target, rhs, "");
        free(rhs);
        char *r = malloc(64);
        strncpy(r, target, 63); r[63] = '\0';
        return r;
    }

    if (node->kind == NODE_CALL) {
        int argCount = node->childCount;
        for (int i = 0; i < argCount; i++) {
            char *argVal = generateIRExpr(prog, node->children[i]);
            emit(prog, IR_ARG, "", argVal, "");
            free(argVal);
        }
        char *tmp = newTemp(prog);
        char  countStr[16];
        snprintf(countStr, sizeof(countStr), "%d", argCount);
        emit(prog, IR_CALL, tmp, node->value, countStr);
        return tmp;
    }

    char comment[256];
    snprintf(comment, sizeof(comment),
             "unsupported expr node kind=%d value='%s'", node->kind, node->value);
    emit(prog, IR_COMMENT, "", comment, "");
    return newTemp(prog);
}

static void generateIRStmt(IRProgram *prog, ASTNode *node) {
    if (!node) return;

    switch (node->kind) {

    case NODE_PROGRAM:
        for (int i = 0; i < node->childCount; i++)
            generateIRStmt(prog, node->children[i]);
        break;

    case NODE_MAIN_DECL:
        emit(prog, IR_LABEL, "main", "", "");
        generateIRStmt(prog, node->children[0]);
        break;

    case NODE_FUNC_DECL:
        emit(prog, IR_LABEL, node->value, "", "");
        for (int i = 0; i < node->childCount - 1; i++) {
            char comment[256];
            snprintf(comment, sizeof(comment), "param %s", node->children[i]->value);
            emit(prog, IR_COMMENT, "", comment, "");
        }
        generateIRStmt(prog, node->children[node->childCount - 1]);
        break;

    case NODE_BLOCK:
        for (int i = 0; i < node->childCount; i++)
            generateIRStmt(prog, node->children[i]);
        break;

    case NODE_VAR_DECL:
        if (node->childCount > 0) {
            char *rhs = generateIRExpr(prog, node->children[0]);
            emit(prog, IR_COPY, node->value, rhs, "");
            free(rhs);
        }
        break;

    case NODE_EXPR_STMT:
        if (node->childCount > 0) {
            char *r = generateIRExpr(prog, node->children[0]);
            free(r);
        }
        break;

    case NODE_OUTPUT:
        if (node->childCount > 0) {
            char *val = generateIRExpr(prog, node->children[0]);
            emit(prog, IR_PRINT, "", val, "");
            free(val);
        }
        break;

    case NODE_ERROR_STMT:
        if (node->childCount > 0) {
            char *val = generateIRExpr(prog, node->children[0]);
            emit(prog, IR_ERROR_PRINT, "", val, "");
            free(val);
        }
        break;

    case NODE_RETURN:
        if (node->childCount > 0) {
            char *val = generateIRExpr(prog, node->children[0]);
            emit(prog, IR_RETURN, "", val, "");
            free(val);
        } else {
            emit(prog, IR_RETURN, "", "", "");
        }
        break;

    case NODE_BREAK:
        if (loopTop < 0)
            emit(prog, IR_COMMENT, "", "break outside loop!", "");
        else
            emit(prog, IR_GOTO, "", loopStack[loopTop].endLabel, "");
        break;

    case NODE_CONTINUE:
        if (loopTop < 0)
            emit(prog, IR_COMMENT, "", "continue outside loop!", "");
        else
            emit(prog, IR_GOTO, "", loopStack[loopTop].startLabel, "");
        break;

    case NODE_IF: {
        char *cond   = generateIRExpr(prog, node->children[0]);
        char *L_else = newLabel(prog);
        char *L_end  = newLabel(prog);
        int   hasElse = (node->childCount >= 3);
        emit(prog, IR_IF_FALSE, "", cond, hasElse ? L_else : L_end);
        free(cond);
        generateIRStmt(prog, node->children[1]);
        if (hasElse) {
            emit(prog, IR_GOTO,  "", L_end,  "");
            emit(prog, IR_LABEL, L_else, "", "");
            generateIRStmt(prog, node->children[2]);
        }
        emit(prog, IR_LABEL, L_end, "", "");
        free(L_else);
        free(L_end);
        break;
    }

    case NODE_WHILE: {
        char *L_start = newLabel(prog);
        char *L_end   = newLabel(prog);
        pushLoop(L_start, L_end);
        emit(prog, IR_LABEL, L_start, "", "");
        char *cond = generateIRExpr(prog, node->children[0]);
        emit(prog, IR_IF_FALSE, "", cond, L_end);
        free(cond);
        generateIRStmt(prog, node->children[1]);
        emit(prog, IR_GOTO,  "", L_start, "");
        emit(prog, IR_LABEL, L_end, "", "");
        popLoop();
        free(L_start);
        free(L_end);
        break;
    }

    case NODE_FOR: {
        char *L_start  = newLabel(prog);
        char *L_update = newLabel(prog);
        char *L_end    = newLabel(prog);
        generateIRStmt(prog, node->children[0]);
        pushLoop(L_update, L_end);
        emit(prog, IR_LABEL, L_start, "", "");
        char *cond = generateIRExpr(prog, node->children[1]);
        emit(prog, IR_IF_FALSE, "", cond, L_end);
        free(cond);
        generateIRStmt(prog, node->children[3]);
        emit(prog, IR_LABEL, L_update, "", "");
        if (node->children[2]->kind != NODE_EXPR_STMT ||
            node->children[2]->childCount > 0) {
            char *upd = generateIRExpr(prog, node->children[2]);
            free(upd);
        }
        emit(prog, IR_GOTO,  "", L_start, "");
        emit(prog, IR_LABEL, L_end, "", "");
        popLoop();
        free(L_start);
        free(L_update);
        free(L_end);
        break;
    }

    case NODE_INCLUDE: {
        char comment[256];
        snprintf(comment, sizeof(comment), "include %s", node->value);
        emit(prog, IR_COMMENT, "", comment, "");
        break;
    }

    default: {
        char comment[256];
        snprintf(comment, sizeof(comment),
                 "unsupported stmt node kind=%d value='%s'", node->kind, node->value);
        emit(prog, IR_COMMENT, "", comment, "");
        break;
    }

    }
}

IRProgram *generateIR(ASTNode *root) {
    IRProgram *prog = createIRProgram();
    loopTop = -1;
    generateIRStmt(prog, root);
    return prog;
}

static void formatInstruction(const IRInstruction *ins, char *buf, size_t bufLen) {
    const char *r  = ins->result;
    const char *o1 = ins->operand1;
    const char *o2 = ins->operand2;
    switch (ins->op) {
    case IR_ADD:         snprintf(buf, bufLen, "%s = %s + %s",   r, o1, o2); break;
    case IR_SUB:         snprintf(buf, bufLen, "%s = %s - %s",   r, o1, o2); break;
    case IR_MUL:         snprintf(buf, bufLen, "%s = %s * %s",   r, o1, o2); break;
    case IR_DIV:         snprintf(buf, bufLen, "%s = %s / %s",   r, o1, o2); break;
    case IR_MOD:         snprintf(buf, bufLen, "%s = %s %% %s",  r, o1, o2); break;
    case IR_EQ:          snprintf(buf, bufLen, "%s = %s == %s",  r, o1, o2); break;
    case IR_NEQ:         snprintf(buf, bufLen, "%s = %s != %s",  r, o1, o2); break;
    case IR_LT:          snprintf(buf, bufLen, "%s = %s < %s",   r, o1, o2); break;
    case IR_GT:          snprintf(buf, bufLen, "%s = %s > %s",   r, o1, o2); break;
    case IR_LEQ:         snprintf(buf, bufLen, "%s = %s <= %s",  r, o1, o2); break;
    case IR_GEQ:         snprintf(buf, bufLen, "%s = %s >= %s",  r, o1, o2); break;
    case IR_AND:         snprintf(buf, bufLen, "%s = %s && %s",  r, o1, o2); break;
    case IR_OR:          snprintf(buf, bufLen, "%s = %s || %s",  r, o1, o2); break;
    case IR_NEG:         snprintf(buf, bufLen, "%s = -%s",       r, o1);     break;
    case IR_NOT:         snprintf(buf, bufLen, "%s = !%s",       r, o1);     break;
    case IR_COPY:        snprintf(buf, bufLen, "%s = %s",        r, o1);     break;
    case IR_LABEL:       snprintf(buf, bufLen, "%s:",            r);          break;
    case IR_GOTO:        snprintf(buf, bufLen, "goto %s",        o1);         break;
    case IR_IF_FALSE:    snprintf(buf, bufLen, "ifFalse %s goto %s", o1, o2); break;
    case IR_PRINT:       snprintf(buf, bufLen, "print %s",       o1);         break;
    case IR_ERROR_PRINT: snprintf(buf, bufLen, "error_print %s", o1);         break;
    case IR_INPUT:       snprintf(buf, bufLen, "%s = input",     r);          break;
    case IR_ARG:         snprintf(buf, bufLen, "arg %s",         o1);         break;
    case IR_CALL:        snprintf(buf, bufLen, "%s = call %s, %s", r, o1, o2); break;
    case IR_RETURN:      snprintf(buf, bufLen, "return %s",      o1);         break;
    case IR_INC:         snprintf(buf, bufLen, "%s++",           r);          break;
    case IR_DEC:         snprintf(buf, bufLen, "%s--",           r);          break;
    case IR_COMMENT:     snprintf(buf, bufLen, "; %s",           o1);         break;
    default:             snprintf(buf, bufLen, "; <unknown op %d>", ins->op); break;
    }
}

static int needsIndent(IROp op) {
    return (op != IR_LABEL && op != IR_COMMENT);
}

void printIR(IRProgram *prog) {
    if (!prog) return;
    char buf[256];
    for (IRInstruction *ins = prog->head; ins; ins = ins->next) {
        formatInstruction(ins, buf, sizeof(buf));
        if (needsIndent(ins->op))
            printf("    %s\n", buf);
        else
            printf("%s\n", buf);
    }
}

int writeIRToFile(IRProgram *prog, const char *path) {
    if (!prog || !path) return -1;
    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "IR: cannot open '%s' for writing\n", path);
        return -1;
    }
    char buf[256];
    for (IRInstruction *ins = prog->head; ins; ins = ins->next) {
        formatInstruction(ins, buf, sizeof(buf));
        if (needsIndent(ins->op))
            fprintf(f, "    %s\n", buf);
        else
            fprintf(f, "%s\n", buf);
    }
    fclose(f);
    return 0;
}