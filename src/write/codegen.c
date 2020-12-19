#include "write.h"
#include "../parser/parser.h"

void compile_node(Node *node) {
    if (node->kind == ND_VAR) {
        printf("  push %d\n", node->val);
        return;
    }
    compile_node(node->lhs);
    compile_node(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    if (node->kind == ND_ADD) {
        printf("  add rax, rdi\n");
    }

    printf("  push rax\n");
}

void codegen() {
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    compile_node(head_node);
    printf("  pop rax\n");
    printf("  ret\n");
}