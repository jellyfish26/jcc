#include "write.h"

void gen_compare(char *comp) {
    printf("  cmp rax, rdi\n");
    printf("  %s al\n", comp);
    printf("  movzx rax, al\n");
}

void gen_var(Node *node) {
    if (node->kind != ND_VAR) {
        errorf(ER_COMPILE, "Not variable");
    }

    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->var->offset);
    printf("  push rax\n");
}