#include "write.h"


void gen_compare(char *comp) {
    printf("  cmp rax, rdi\n");
    printf("  %s al\n", comp);
    printf("  movzx rax, al\n");
}