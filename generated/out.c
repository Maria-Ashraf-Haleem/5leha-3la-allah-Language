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
    double t2 = score > 10;
    if (!(t2)) goto L2;
    printf("%s\n", "score is greater than 10");
    printf("%g\n", score);
L2: ;
    printf("%s\n", "done");
    return 0;
}
