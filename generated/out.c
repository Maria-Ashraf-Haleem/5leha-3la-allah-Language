#include <stdio.h>
#include <stdlib.h>

double hat(void) {
    double x;
    scanf("%lf", &x);
    return x;
}

int main(void) {
    double x = 20;
    double t1 = x > 10;
    if (!(t1)) goto L2;
    printf("%s\n", "x is big");
    printf("%g\n", x);
L2: ;
    printf("%s\n", "finished");
    return 0;
}
