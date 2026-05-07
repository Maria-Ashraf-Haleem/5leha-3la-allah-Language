#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/codegen.h"
#include "../include/ir.h"

#define MAX_DECLARED 1000
#define MAX_NAME_LEN 64

static char declared[MAX_DECLARED][MAX_NAME_LEN];
static int declaredCount = 0;

static int isDeclared(const char *name) {
    for (int i = 0; i < declaredCount; i++) {
        if (strcmp(declared[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

static void markDeclared(const char *name) {
    if (!name || name[0] == '\0') {
        return;
    }

    if (declaredCount >= MAX_DECLARED) {
        fprintf(stderr, "codegen: declared-name table full, skipping '%s'\n", name);
        return;
    }

    strncpy(declared[declaredCount], name, MAX_NAME_LEN - 1);
    declared[declaredCount][MAX_NAME_LEN - 1] = '\0';
    declaredCount++;
}

static void resetDeclared(void) {
    declaredCount = 0;
}

static int isStringLiteral(const char *s) {
    if (!s) {
        return 0;
    }

    int len = (int)strlen(s);
    return len >= 2 && s[0] == '"' && s[len - 1] == '"';
}

static void writeCStringLiteral(FILE *f, const char *s) {
    int len = (int)strlen(s);
    const char *inner = s + 1;
    int innerLen = len - 2;

    fputc('"', f);

    for (int i = 0; i < innerLen; i++) {
        char c = inner[i];

        switch (c) {
            case '"':
                fputs("\\\"", f);
                break;
            case '\\':
                fputs("\\\\", f);
                break;
            case '\n':
                fputs("\\n", f);
                break;
            case '\t':
                fputs("\\t", f);
                break;
            default:
                fputc(c, f);
                break;
        }
    }

    fputc('"', f);
}

static void emitAssignmentPrefix(FILE *f, const char *name) {
    if (!name || name[0] == '\0') {
        fprintf(f, "    ");
        return;
    }

    if (!isDeclared(name)) {
        markDeclared(name);
        fprintf(f, "    double %s = ", name);
    } else {
        fprintf(f, "    %s = ", name);
    }
}

int generateCFromIR(IRProgram *program, const char *outputPath) {
    if (!program || !outputPath) {
        return -1;
    }

    FILE *f = fopen(outputPath, "w");
    if (!f) {
        fprintf(stderr, "codegen: cannot open '%s' for writing\n", outputPath);
        return -1;
    }

    resetDeclared();

    int mainOpened = 0;
    int returnEmitted = 0;

    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <stdlib.h>\n\n");

    fprintf(f, "double hat(void) {\n");
    fprintf(f, "    double x;\n");
    fprintf(f, "    scanf(\"%%lf\", &x);\n");
    fprintf(f, "    return x;\n");
    fprintf(f, "}\n\n");

    for (IRInstruction *ins = program->head; ins; ins = ins->next) {
        const char *r = ins->result;
        const char *o1 = ins->operand1;
        const char *o2 = ins->operand2;

        switch (ins->op) {
            case IR_LABEL:
                if (strcmp(r, "main") == 0) {
                    fprintf(f, "int main(void) {\n");
                    mainOpened = 1;
                } else {
                    fprintf(f, "%s: ;\n", r);
                }
                break;

            case IR_ADD:
                emitAssignmentPrefix(f, r);
                fprintf(f, "%s + %s;\n", o1, o2);
                break;

            case IR_SUB:
                emitAssignmentPrefix(f, r);
                fprintf(f, "%s - %s;\n", o1, o2);
                break;

            case IR_MUL:
                emitAssignmentPrefix(f, r);
                fprintf(f, "%s * %s;\n", o1, o2);
                break;

            case IR_DIV:
                emitAssignmentPrefix(f, r);
                fprintf(f, "%s / %s;\n", o1, o2);
                break;

            case IR_MOD:
                emitAssignmentPrefix(f, r);
                fprintf(f, "(int)%s %% (int)%s;\n", o1, o2);
                break;

            case IR_EQ:
                emitAssignmentPrefix(f, r);
                fprintf(f, "%s == %s;\n", o1, o2);
                break;

            case IR_NEQ:
                emitAssignmentPrefix(f, r);
                fprintf(f, "%s != %s;\n", o1, o2);
                break;

            case IR_LT:
                emitAssignmentPrefix(f, r);
                fprintf(f, "%s < %s;\n", o1, o2);
                break;

            case IR_GT:
                emitAssignmentPrefix(f, r);
                fprintf(f, "%s > %s;\n", o1, o2);
                break;

            case IR_LEQ:
                emitAssignmentPrefix(f, r);
                fprintf(f, "%s <= %s;\n", o1, o2);
                break;

            case IR_GEQ:
                emitAssignmentPrefix(f, r);
                fprintf(f, "%s >= %s;\n", o1, o2);
                break;

            case IR_AND:
                emitAssignmentPrefix(f, r);
                fprintf(f, "%s && %s;\n", o1, o2);
                break;

            case IR_OR:
                emitAssignmentPrefix(f, r);
                fprintf(f, "%s || %s;\n", o1, o2);
                break;

            case IR_NEG:
                emitAssignmentPrefix(f, r);
                fprintf(f, "-%s;\n", o1);
                break;

            case IR_NOT:
                emitAssignmentPrefix(f, r);
                fprintf(f, "!%s;\n", o1);
                break;

            case IR_COPY:
                emitAssignmentPrefix(f, r);
                if (isStringLiteral(o1)) {
                    fprintf(f, "0;\n");
                } else {
                    fprintf(f, "%s;\n", o1);
                }
                break;

            case IR_INPUT:
                emitAssignmentPrefix(f, r);
                fprintf(f, "hat();\n");
                break;

            case IR_INC:
                fprintf(f, "    %s++;\n", r);
                break;

            case IR_DEC:
                fprintf(f, "    %s--;\n", r);
                break;

            case IR_GOTO:
                fprintf(f, "    goto %s;\n", o1);
                break;

            case IR_IF_FALSE:
                fprintf(f, "    if (!(%s)) goto %s;\n", o1, o2);
                break;

            case IR_PRINT:
                if (isStringLiteral(o1)) {
                    fprintf(f, "    printf(\"%%s\\n\", ");
                    writeCStringLiteral(f, o1);
                    fprintf(f, ");\n");
                } else {
                    fprintf(f, "    printf(\"%%g\\n\", %s);\n", o1);
                }
                break;

            case IR_ERROR_PRINT:
                if (isStringLiteral(o1)) {
                    fprintf(f, "    fprintf(stderr, \"%%s\\n\", ");
                    writeCStringLiteral(f, o1);
                    fprintf(f, ");\n");
                } else {
                    fprintf(f, "    fprintf(stderr, \"%%g\\n\", %s);\n", o1);
                }
                break;

            case IR_RETURN:
                if (!returnEmitted) {
                    fprintf(f, "    return 0;\n");
                    returnEmitted = 1;
                }
                break;

            case IR_ARG:
                break;

            case IR_CALL:
                emitAssignmentPrefix(f, r);
                fprintf(f, "0;\n");
                break;

            case IR_COMMENT:
                break;

            default:
                break;
        }
    }

    if (mainOpened) {
        if (!returnEmitted) {
            fprintf(f, "    return 0;\n");
        }
        fprintf(f, "}\n");
    }

    fclose(f);
    return 0;
}