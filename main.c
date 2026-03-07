#include <stdio.h>
#include "scanner.h"

int main() {
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

    scan(source);
    printTokens();
    return 0;
}