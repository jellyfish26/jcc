#include <stdio.h>

void foo() {
    printf("foo");
}

void hoge(long int a1, long int a2, long int a3, long int a4, long int a5, long int a6) {
    printf("%ld, %ld, %ld, %ld, %ld, %ld", a1, a2, a3, a4, a5, a6);
}

void eight_print(long int a1, long int a2, long int a3, long int a4, long int a5, long int a6, long int a7, long int a8) {
    printf("%ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld", a1, a2, a3, a4, a5, a6, a7, a8);
}