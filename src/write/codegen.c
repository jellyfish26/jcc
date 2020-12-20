#include "write.h"
#include "../parser/parser.h"

void compile_node(Node *node) {
    if (node->kind == ND_INT) {
        printf("  push %d\n", node->val);
        return;
    }
    compile_node(node->lhs);
    compile_node(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    // calculation
    switch (node->kind) {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv rax, rdi\n");
        break;
    case ND_EQ:
        gen_compare("sete");
        break;
    case ND_NEQ:
        gen_compare("setne");
        break;
    case ND_LC:
        gen_compare("setl");
        break;
    case ND_LEC:
        gen_compare("setle");
        break;
    case ND_RC:
        gen_compare("setg");
        break;
    case ND_REC:
        gen_compare("setge");
        break;
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