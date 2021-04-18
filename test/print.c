#include <stdio.h>

void foo() {
    printf("foo");
}

void hi(long *a) {
    printf("%x\n", a);
}

void hoge(int a1, int a2, int a3, int a4, int a5, int a6) {
    printf("%d, %d, %d, %d, %d, %d", a1, a2, a3, a4, a5, a6);
}

void eight_print(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) {
    printf("%d, %d, %d, %d, %d, %d, %d, %d", a1, a2, a3, a4, a5, a6, a7, a8);
}
