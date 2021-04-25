#include "write.h"

#include <stdio.h>

int label = 0;
char *args_64reg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
char *args_32reg[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};

void compile_node(Node *node);
void expand_logical_and(Node *node, int label);
void expand_logical_or(Node *node, int label);

void expand_variable(Node *node) {
  gen_var(node);
  printf("  pop rax\n");
  TypeKind var_type_kind = node->var->var_type->kind;
  if (var_type_kind == TY_INT) {
    printf("  mov eax, DWORD PTR [rax]\n");
  } else if (var_type_kind == TY_LONG || var_type_kind == TY_PTR) {
    printf("  mov rax, QWORD PTR [rax]\n");
  }
  printf("  push rax\n");
}

void expand_assign(Node *node) {
  TypeKind var_type_kind = TY_PTR;
  if (node->lhs->kind == ND_VAR) {
    gen_var(node->lhs);
    var_type_kind = node->lhs->var->var_type->kind;
  } else if (node->lhs->kind == ND_CONTENT) {
    compile_node(node->lhs->lhs);
    if (node->lhs->var) {
      var_type_kind = node->lhs->var->var_type->kind;
    }
  } else {
    errorf(ER_COMPILE, "Cannot assign");
  }
  compile_node(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  if (var_type_kind == TY_INT) {
    printf("  mov DWORD PTR [rax], edi\n");
  } else if (var_type_kind == TY_LONG || var_type_kind == TY_PTR) {
    printf("  mov QWORD PTR [rax], rdi\n");
  }
  printf("  push rdi\n");
}

void expand_logical_and(Node *node, int label) {
    compile_node(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lfalse%d\n", label);
  if (node->rhs && node->rhs->kind == ND_LOGICALAND) {
    expand_logical_and(node->rhs, label);
  } else {
    compile_node(node->rhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lfalse%d\n", label);
  }
}

void expand_logical_or(Node *node, int label) {
  compile_node(node->lhs);
  printf("  pop rax\n");
  printf("  cmp rax, 0\n");
  printf("  jne .Ltrue%d\n", label);
  if (node->rhs && node->rhs->kind == ND_LOGICALAND) {
    expand_logical_or(node->rhs, label);
  } else {
    compile_node(node->rhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  jne .Ltrue%d\n", label);
  }
}

void expand_ternary(Node *node, int label) {
  compile_node(node->exec_if);
  printf("  pop rax\n");
  printf("  cmp rax, 0\n");
  printf("  je .Lfalse%d\n", label);
  compile_node(node->lhs);
  printf("  jmp .Lnext%d\n", label);
  printf(".Lfalse%d:\n", label);
  compile_node(node->rhs);
  printf(".Lnext%d:\n", label);
}

void compile_node(Node *node) {
  if (node->kind == ND_INT) {
    printf("  push %d\n", node->val);
    return;
  }

  if (node->kind == ND_BLOCK) {
    for (Node *now_stmt = node->next_stmt; now_stmt;
         now_stmt = now_stmt->next_stmt) {
      compile_node(now_stmt);
    }
    return;
  }

  switch (node->kind) {
    case ND_VAR:
      expand_variable(node);
      return;
    case ND_ADDR:
      if (node->lhs->kind == ND_VAR) {
        gen_var(node->lhs);
      } else if (node->lhs->kind == ND_CONTENT) {
        compile_node(node->lhs->lhs);
      }
      return;
    case ND_ASSIGN:
      expand_assign(node);
      return;
    case ND_RETURN:
      compile_node(node->lhs);
      printf("  pop rax\n");
      printf("  mov rsp, rbp\n");
      printf("  pop rbp\n");
      printf("  ret\n");
      return;
    case ND_IF: {
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
    case ND_TERNARY: {
      int now_label = label++;
      expand_ternary(node, label);
      return;
    }
    case ND_WHILE:
    case ND_FOR: {
      int now_label = label++;
      node->label = now_label;
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
    case ND_LOOPBREAK: {
      int now_label = node->lhs->label;
      printf("  jmp .Lend%d\n", now_label);
      return;
    }
    case ND_CONTINUE: {
      int now_label = node->lhs->label;
      printf("  jmp .Lbegin%d\n", now_label);
      return;
    }
    case ND_CONTENT: {
      compile_node(node->lhs);
      if (!node->var || (node->var && node->var->var_type->kind != TY_ARRAY)) {
        printf("  pop rax\n");
        printf("  mov rax, QWORD PTR [rax]\n");
        printf("  push rax\n");
      }
      return;
    }
    case ND_LOGICALAND: {
        int now_label = label++;
        expand_logical_and(node, now_label);
        printf("  mov rax, 1\n");
        printf("  jmp .Lnext%d\n", now_label);
        printf(".Lfalse%d:\n", now_label);
        printf("  mov rax, 0\n");
        printf(".Lnext%d:\n", now_label);
        printf("  push rax\n");
        return;
      }
    case ND_LOGICALOR: {
        int now_label = label++;
        expand_logical_or(node, now_label);
        printf("  mov rax, 0\n");
        printf("  jmp .Lnext%d\n", now_label);
        printf(".Ltrue%d:\n", now_label);
        printf("  mov rax, 1\n");
        printf(".Lnext%d:\n", now_label);
        printf("  push rax\n");
        return;
      }
  }

  if (node->kind == ND_FUNCCALL) {
    char *name = calloc(node->func_name_len + 1, sizeof(char));
    memcpy(name, node->func_name, node->func_name_len);
    int arg_count = 0;
    for (Node *now_arg = node->func_arg; now_arg; now_arg = now_arg->func_arg) {
      compile_node(now_arg);
      arg_count++;
    }

    for (int arg_idx = 0; arg_idx < arg_count && arg_idx < 6; arg_idx++) {
      printf("  pop %s\n", args_64reg[arg_idx]);
    }

    printf("  call %s\n", name);
    printf("  push rax\n");
    return;
  }

  compile_node(node->lhs);
  compile_node(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  TypeKind formula_type_kind;
  if (node->lhs->var) {
    formula_type_kind = node->lhs->var->var_type->kind;
  } else {
    formula_type_kind = TY_INT;
  }

  if (node->lhs->var && node->lhs->var->var_type->move_size != 1) {
    printf("  imul rdi, %d\n", node->lhs->var->var_type->move_size);
  }

  if (node->rhs->var && node->rhs->var->var_type->move_size != 1) {
    printf("  imul rax, %d\n", node->rhs->var->var_type->move_size);
  }

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
      switch (formula_type_kind) {
        case TY_INT:
          printf("  cdq\n");
          printf("  idiv eax, edi\n");
          break;
        case TY_LONG:
          printf("  cqo\n");
          printf("  idiv rax, rdi\n");
          break;
      }
      break;
    case ND_REMAINDER:
      switch (formula_type_kind) {
        case TY_INT:
          printf("  cdq\n");
          printf("  idiv edi\n");
          printf("  mov eax, edx\n");
          break;
        case TY_LONG:
          printf("  cqo\n");
          printf("  idiv rdi\n");
          printf("  mov rax, rdx\n");
          break;
      }
      break;
    case ND_LEFTSHIFT:
      printf("  mov rcx, rdi\n");
      printf("  sal rax, cl\n");
      break;
    case ND_RIGHTSHIFT:
      printf("  mov rcx, rdi\n");
      printf("  sar rax, cl\n");
      break;
    case ND_BITWISEAND:
      printf("  and rax, rdi\n");
      break;
    case ND_BITWISEXOR:
      printf("  xor rax, rdi\n");
      break;
    case ND_BITWISEOR:
      printf("  or rax, rdi\n");
      break;
    case ND_EQ:
      gen_compare("sete", formula_type_kind);
      break;
    case ND_NEQ:
      gen_compare("setne", formula_type_kind);
      break;
    case ND_LC:
      gen_compare("setl", formula_type_kind);
      break;
    case ND_LEC:
      gen_compare("setle", formula_type_kind);
      break;
    case ND_RC:
      gen_compare("setg", formula_type_kind);
      break;
    case ND_REC:
      gen_compare("setge", formula_type_kind);
      break;
  }

  printf("  push rax\n");
}

void codegen() {
  printf(".intel_syntax noprefix\n");

  for (Function *now_func = top_func; now_func; now_func = now_func->next) {
    init_offset(now_func);
    char *func_name = calloc(now_func->func_name_len + 1, sizeof(char));
    memcpy(func_name, now_func->func_name, now_func->func_name_len);
    printf(".global %s\n", func_name);
    printf("%s:\n", func_name);

    // Prologue
    // Allocate variable size.
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", now_func->vars_size);

    // Set arguments (use register)
    int arg_count = now_func->func_argc - 1;
    for (Node *arg = now_func->func_args; arg; arg = arg->lhs) {
      if (arg_count < 6) {
        gen_var(arg);
        printf("  pop rax\n");
        switch (arg->var->var_type->kind) {
        case TY_INT:
          printf("  mov DWORD PTR [rax], %s\n", args_32reg[arg_count]);
          break;
        case TY_LONG:
          printf("  mov QWORD PTR [rax], %s\n", args_64reg[arg_count]);
          break;
        }
      }
      arg_count--;
    }

    // Set arguements (use stack due more than 7 arguments)
    arg_count = now_func->func_argc - 1;
    for (Node *arg = now_func->func_args; arg; arg = arg->lhs) {
      if (arg_count >= 6) {
        gen_var(arg);
        printf("  mov rax, [rbp + %d]\n", 8 + (arg_count - 5) * 8);
        printf("  pop rsi\n");
        switch (arg->var->var_type->kind) {
        case TY_INT:
          printf("  mov DWORD PTR [rsi], eax\n");
          break;
        case TY_LONG:
          printf("  mov QWORD PTR [rsi], rax\n");
          break;
        }
      }
      arg_count--;
    }

    compile_node(now_func->stmt);

    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret \n");
  }
}
