#include "write.h"

void gen_compare(char *comp, TypeKind var_type_kind) {
    if (var_type_kind == TY_INT) {
        printf("  cmp eax, edi\n");
    } else if (var_type_kind == TY_LONG) {
        printf("  cmp rax, rdi\n");
    }
    
    printf("  %s al\n", comp);
    printf("  movzx eax, al\n");
}

void gen_var(Node *node) {
    if (node->kind != ND_VAR && node->kind != ND_ADDR) {
        errorf(ER_COMPILE, "Not variable");
    }

    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->var->offset);
    printf("  push rax\n");
}