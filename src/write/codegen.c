#include "write.h"

int label = 0;

void compile_node(Node *node) {
    if (node->kind == ND_INT) {
        printf("  push %d\n", node->val);
        return;
    }

    if (node->kind == ND_BLOCK) {
        for (Node *now_stmt = node->next_stmt; now_stmt; now_stmt = now_stmt->next_stmt) {
            compile_node(now_stmt);
        }
        return;
    }

    switch (node->kind) {
    case ND_VAR:
        gen_var(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    case ND_ASSIGN:
        gen_var(node->lhs);
        compile_node(node->rhs);

        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    case ND_RETURN:
        compile_node(node->lhs);
        printf("  pop rax\n");
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
        return;
    case ND_IF:
        {
            int now_label = label++;
            compile_node(node->judge);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je .Lelse%d\n", now_label);

            // "true"
            compile_node(node->exec_if);
            printf("  jmp .Lend%d\n", now_label);

            printf(".Lelse%d:\n", now_label);
            // "else" statement
            if (node->exec_else) {
                compile_node(node->exec_else);
            }

            // continue
            printf(".Lend%d:\n", now_label);
            return;
        }
    case ND_FOR:
        {
            int now_label = label++;
            compile_node(node->judge);

            if (node->init_for) {
                compile_node(node->init_for);
            }

            printf(".Lbegin%d:\n", now_label);

            // judege expr
            if (node->judge) {
                compile_node(node->judge);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je .Lend%d\n", now_label);
            }

            compile_node(node->stmt_for);

            // repeat expr
            if (node->repeat_for) {
                compile_node(node->repeat_for);
            }

            // finally
            printf("  jmp .Lbegin%d\n", now_label);
            printf(".Lend%d:\n", now_label);
            return;
        }
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

    // Prologue
    // Allocate variable size.
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", vars_size);

    for (int i = 0; code[i]; i++) {
        compile_node(code[i]);
    }

    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret \n");
}