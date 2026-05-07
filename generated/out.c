#include <stdio.h>
#include <stdlib.h>

double hat(void) {
    double x;
    scanf("%lf", &x);
    return x;
}

int main(void) {
    double t1 = 10 + 5;
    double score = t1;
    double pi = 3.14;
    double t2 = score == 15;
    double t3 = score != 0;
    double t4 = t2 && t3;
    if (!(t4)) goto L2;
    printf("%s\n", "hello");
L2: ;
    double t5 = score <= 20;
    double t6 = score >= 5;
    double t7 = t5 || t6;
    if (!(t7)) goto L4;
    printf("%s\n", "he said \"hi\"");
    return 0;
L4: ;
}
