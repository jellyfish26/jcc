#include "code/codegen.h"
#include "parser/parser.h"
#include "token/tokenize.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// About assembly
//

static const char *reg_8byte[] = {
  "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
  "QWORD PTR [rax]"
};

static const char *reg_4byte[] = {
  "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
  "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
  "DWORD PTR [rax]"
};

static const char *reg_2byte[] = {
  "ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
  "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
  "WORD PTR [rax]"
};

static const char *reg_1byte[] = {
  "al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl",
  "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
  "BYTE PTR [rax]"
};

void compile_node(Node *node);

static const char *get_reg(RegKind reg, int reg_size) {
  switch (reg_size) {
    case 1:
      return reg_1byte[reg];
    case 2:
      return reg_2byte[reg];
    case 4:
      return reg_4byte[reg];
    case 8:
      return reg_8byte[reg];
    default:
      return reg_8byte[reg];
  }
}

int get_type_size(Type *type) {
  switch (type->kind) {
    case TY_CHAR:
      return 1;
    case TY_SHORT:
      return 2;
    case TY_INT:
      return 4;
    case TY_LONG:
      return 8;
    default:
      return 8;
  }
}

void gen_compare(char *comp_op, int reg_size) {
  printf("  cmp %s, %s\n", get_reg(REG_RAX, reg_size), get_reg(REG_RDI, reg_size));
  printf("  %s al\n", comp_op);
  printf("  movzx rax, al\n");
}

int push_cnt = 0;

void gen_push(RegKind reg) {
  push_cnt++;
  printf("  push %s\n", get_reg(reg, 8));
}

void gen_pop(RegKind reg) {
  push_cnt--;
  printf("  pop %s\n", get_reg(reg, 8));
}

void gen_emptypop(int num) {
  push_cnt -= num;
  printf("  add rsp, %d\n", num * 8);
}

// OP_MOV: left_reg = right_reg
// OP_MOVSX: left_reg = right_reg (Move with Sign-Extension, Size of left_reg is REG_SIZE_4)
// OP_ADD: left_reg = left_reg + right_reg
// OP_SUB: left_reg = left_reg - right_reg
// OP_MUL: left_reg = left_reg * right_reg (Cannot use REG_MEM both left_reg and right_reg)
// OP_DIV: left_reg = left_reg / right_reg (Overwrite rax and rdx registers)
// OP_REMAINDER: left_reg = left_reg % right_reg (Overwrite rax and rdx registers)
// OP_BITWISE_SHIFT_LEFT: left_reg = left_reg << right_reg (Overwrite rcx register)
// OP_BITWISE_SHIFT_RIGHT: left_reg = left_reg >> right_reg (overwrite rcx register)
// OP_BITWISE_(AND | XOR | OR | NOT): left_reg = left_reg = (and | xor | or | not) right_reg
bool gen_operation(RegKind left_reg, RegKind right_reg, int reg_size, OpKind op) {

  // normal operation
  switch (op) {
    case OP_MOV:
      if (left_reg == REG_MEM || reg_size >= 4) {
        printf("  mov %s, %s\n", get_reg(left_reg, reg_size), get_reg(right_reg, reg_size));
      } else {
        printf("  movzx %s, %s\n", get_reg(left_reg, 8), get_reg(right_reg, reg_size));
      }
      return true;
    case OP_MOVSX:
      printf("  movsx %s, %s\n", get_reg(left_reg, 4), get_reg(right_reg, reg_size));
      return true;
    case OP_ADD:
      printf("  add %s, %s\n", get_reg(left_reg, reg_size), get_reg(right_reg, reg_size));
      return true;
    case OP_SUB:
      printf("  sub %s, %s\n", get_reg(left_reg, reg_size), get_reg(right_reg, reg_size));
      return true;
    case OP_MUL: {
      if (left_reg == REG_MEM) {
        return false;
      }
      if (reg_size == 1) {
        reg_size = 2;
      }
      printf("  imul %s, %s\n", get_reg(left_reg, reg_size), get_reg(right_reg, reg_size));
      return true;
    }
    case OP_DIV:
    case OP_REMAINDER: {
      if (left_reg == REG_MEM) {
        gen_push(REG_RAX);
      }
      if (reg_size <= 2) {
        gen_operation(REG_RAX, left_reg, reg_size, OP_MOVSX);
      } else if (left_reg != REG_RAX) {
        gen_operation(REG_RAX, left_reg, reg_size, OP_MOV);
      }
      if (reg_size == 8) {
        printf("  cqo\n");
      } else {
        printf("  cdq\n");
      }
      if (reg_size <= 4) {
        printf("  idiv %s\n", get_reg(right_reg, 4));
      } else {
        printf("  idiv %s\n", get_reg(right_reg, reg_size));
      }
      if (op == OP_DIV) {
        gen_operation(REG_RDX, REG_RAX, 8, OP_MOV);
      }
      if (left_reg == REG_MEM) {
        gen_pop(REG_RAX);
      }
      gen_operation(left_reg, REG_RDX, reg_size, OP_MOV);
      return true;
    }
    case OP_LEFT_SHIFT:
    case OP_RIGHT_SHIFT: {
      if (right_reg != REG_RCX) {
        gen_operation(REG_RCX, right_reg, reg_size, OP_MOV);
      }
      if (op == OP_LEFT_SHIFT) {
        printf("  sal %s, %s\n", get_reg(left_reg, reg_size), get_reg(REG_RCX, reg_size));
      } else {
        printf("  sar %s, %s\n", get_reg(left_reg, reg_size), get_reg(REG_RCX, reg_size));
      }
      return true;
    }
    case OP_BITWISE_AND:
      printf("  and %s, %s\n", get_reg(left_reg, reg_size), get_reg(right_reg, reg_size));
      return true;
    case OP_BITWISE_XOR:
      printf("  xor %s, %s\n", get_reg(left_reg, reg_size), get_reg(right_reg, reg_size));
      return true;
    case OP_BITWISE_OR:
      printf("  or %s, %s\n", get_reg(left_reg, reg_size), get_reg(right_reg, reg_size));
      return true;
    case OP_BITWISE_NOT:
      printf("  not %s\n", get_reg(left_reg, reg_size));
      return true;
  }
  return false;
}

int label = 0;

void compile_node(Node *node);
void expand_logical_and(Node *node, int label);
void expand_logical_or(Node *node, int label);

void gen_var_address(Node *node) {
  if (node->kind != ND_VAR && node->kind != ND_ADDR) {
    errorf(ER_COMPILE, "Not variable");
  }
  Obj *var = node->use_var;

  // String literal
  if (var->type->kind == TY_STR) {
    printf("  mov rax, offset .LC%d\n", var->offset);
  } else if (var->is_global) {
    char *var_name = calloc(var->name_len + 1, sizeof(char));
    memcpy(var_name, var->name, var->name_len);
    printf("  mov rax, offset %s\n", var_name);
  } else {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->use_var->offset);
  }
}

void expand_variable(Node *node) {
  gen_var_address(node);
  Type *var_type = node->use_var->type;
  if (var_type->kind != TY_ARRAY && var_type->kind != TY_STR) {
    gen_operation(REG_RAX, REG_MEM, get_type_size(var_type), OP_MOV);
  }
}

// If left node is a direct variable, get the address of the variable.
// If left node is a indirect, get original address.
void gen_assignable_address(Node *node) {
  if (node->kind == ND_VAR) {
    gen_var_address(node);
  } else if (node->kind == ND_CONTENT) {
    compile_node(node->lhs);
  } else {
    errorf(ER_COMPILE, "Cannot assign");
  }
}

// Right to left
void expand_assign(Node *node) {
  switch (node->rhs->kind) {
    case ND_ASSIGN:
      expand_assign(node->rhs);
      break;
    default:
      compile_node(node->rhs);
      gen_operation(REG_RDI, REG_RAX, 8, OP_MOV);
  }
  gen_push(REG_RDI);
  // The left node must be assignable.
  gen_assignable_address(node->lhs);
  gen_pop(REG_RDI);
  int reg_size = get_type_size(node->lhs->type);

  switch (node->assign_type) {
    case ND_ADD: {
      gen_operation(REG_RDI, REG_MEM, reg_size, OP_ADD);
      break;
    }
    case ND_SUB: {
      gen_push(REG_RAX);
      gen_operation(REG_RAX, REG_MEM, reg_size, OP_MOV);
      gen_operation(REG_RAX, REG_RDI, reg_size, OP_SUB);
      gen_operation(REG_RDI, REG_RAX, reg_size, OP_MOV);
      gen_pop(REG_RAX);
      break;
    }
    case ND_MUL: {
      gen_operation(REG_RDI, REG_MEM, reg_size, OP_MUL);
      break;
    }
    case ND_DIV:
    case ND_REMAINDER: {
      gen_push(REG_RAX);
      if (node->assign_type == ND_DIV) {
        gen_operation(REG_MEM, REG_RDI, reg_size, OP_DIV);
      } else {
        gen_operation(REG_MEM, REG_RDI, reg_size, OP_REMAINDER);
      }
      gen_pop(REG_RAX);
      gen_operation(REG_RDI, REG_MEM, reg_size, OP_MOV);
      break;
    }
    case ND_LEFTSHIFT:
      gen_operation(REG_MEM, REG_RDI, reg_size, OP_LEFT_SHIFT);
      gen_operation(REG_RDI, REG_MEM, reg_size, OP_MOV);
      break;
    case ND_RIGHTSHIFT:
      gen_operation(REG_MEM, REG_RDI, reg_size, OP_RIGHT_SHIFT);
      gen_operation(REG_RDI, REG_MEM, reg_size, OP_MOV);
      break;
    case ND_BITWISEAND:
      gen_operation(REG_MEM, REG_RDI, reg_size, OP_BITWISE_AND);
      gen_operation(REG_RDI, REG_MEM, reg_size, OP_MOV);
      break;
    case ND_BITWISEXOR:
      gen_operation(REG_MEM, REG_RDI, reg_size, OP_BITWISE_XOR);
      gen_operation(REG_RDI, REG_MEM, reg_size, OP_MOV);
      break;
    case ND_BITWISEOR:
      gen_operation(REG_MEM, REG_RDI, reg_size, OP_BITWISE_OR);
      gen_operation(REG_RDI, REG_MEM, reg_size, OP_MOV);
      break;
    default:
      break;
  }
  gen_operation(REG_MEM, REG_RDI, reg_size, OP_MOV);
}

void expand_logical_and(Node *node, int label) {
  compile_node(node->lhs);
  printf("  cmp rax, 0\n");
  printf("  je .Lfalse%d\n", label);

  if (node->rhs && node->rhs->kind == ND_LOGICALAND) {
    expand_logical_and(node->rhs, label);
  } else {
    compile_node(node->rhs);
    printf("  cmp rax, 0\n");
    printf("  je .Lfalse%d\n", label);
  }
}

void expand_logical_or(Node *node, int label) {
  compile_node(node->lhs);
  printf("  cmp rax, 0\n");
  printf("  jne .Ltrue%d\n", label);

  if (node->rhs && node->rhs->kind == ND_LOGICALAND) {
    expand_logical_or(node->rhs, label);
  } else {
    compile_node(node->rhs);
    printf("  cmp rax, 0\n");
    printf("  jne .Ltrue%d\n", label);
  }
}

void expand_ternary(Node *node, int label) {
  compile_node(node->exec_if);
  printf("  cmp rax, 0\n");
  printf("  je .Lfalse%d\n", label);

  compile_node(node->lhs);
  printf("  jmp .Lnext%d\n", label);
  printf(".Lfalse%d:\n", label);

  compile_node(node->rhs);
  printf(".Lnext%d:\n", label);
}

const RegKind args_reg[] = {
  REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9
};

static char i16i8[] = "movsx ax, al";
static char i32i8[] = "movsx eax, al";
static char i32i16[] = "movsx eax, ax";
static char i64i8[] = "movsx rax, al";
static char i64i16[] = "movsx rax, ax";
static char i64i32[] = "movsxd rax, eax";
static char u16u8[] = "movzx ax, al";
static char u32u8[] = "movzx eax, al";
static char u32u16[] = "movzx eax, ax";
static char u64u8[] = "movzx rax, al";
static char u64u16[] = "movzx rax, ax";
static char u64u32[] = "mov eax, eax";

static const char *cast_table[][4] = {
  // i8,  i16,    i32,    i64       to/from
  {NULL,  i16i8,  i32i8,  i64i8},   // i8
  {i64i8, NULL,   i32i16, i64i16},  // i16
  {i64i8, i64i16, NULL,   i64i32},  // i32
  {i64i8, i64i16, i64i32, NULL}     // i64
};

static int type_to_cast_table_idx(Type *type) {
  switch (type->kind) {
    case TY_CHAR:
      return 0;
    case TY_SHORT:
      return 1;
    case TY_INT:
      return 2;
    case TY_LONG:
      return 3;
    default:
      return 0;
  };
}

void gen_cast(Node *node) {
  if (node->kind != ND_CAST) {
    return;
  }
  compile_node(node->lhs);
  int from = type_to_cast_table_idx(node->lhs->type);
  int to = type_to_cast_table_idx(node->type);
  if (cast_table[from][to] != NULL) {
    printf("  %s\n", cast_table[from][to]);
  }
};

void compile_node(Node *node) {
  if (node->kind == ND_INT) {
    printf("  mov rax, %d\n", node->val);
    return;
  }

  if (node->kind == ND_CAST) {
    gen_cast(node);
    return;
  } 

  if (node->kind == ND_BLOCK) {
    int stack_cnt = push_cnt;
    for (Node *now_stmt = node->next_block; now_stmt;
         now_stmt = now_stmt->next_stmt) {
      compile_node(now_stmt);
    }
    return;
  }

  switch (node->kind) {
    case ND_VAR:
      if (!node->is_var_define_only) {
        expand_variable(node);
      }
      return;
    case ND_ADDR:
      if (node->lhs->kind == ND_VAR) {
        gen_var_address(node->lhs);
      } else if (node->lhs->kind == ND_CONTENT) {
        compile_node(node->lhs->lhs);
      }
      return;
    case ND_ASSIGN:
      expand_assign(node);
      return;
    case ND_RETURN:
      compile_node(node->lhs);
      printf("  mov rsp, rbp\n");
      gen_pop(REG_RBP);
      printf("  ret\n");
      return;
    case ND_IF: {
      int now_label = label++;
      compile_node(node->judge);
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
        printf("  cmp rax, 0\n");
        printf("  je .Lend%d\n", now_label);
      }

      compile_node(node->stmt_for);

      // repeat expr
      if (node->repeat_for) {
        int stack_cnt = push_cnt;
        compile_node(node->repeat_for);
        if (push_cnt - stack_cnt >= 1) {
          gen_emptypop(push_cnt - stack_cnt);
        }
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
      if (node->type->kind != TY_ARRAY) {
        gen_operation(REG_RAX, REG_MEM, get_type_size(node->type), OP_MOV);
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
      return;
    }
    case ND_PREFIX_INC:
    case ND_PREFIX_DEC: {
      gen_assignable_address(node->lhs);
      printf("  mov rdi, 1\n");
      if (node->kind == ND_PREFIX_INC) {
        gen_operation(REG_MEM, REG_RDI, get_type_size(node->type), OP_ADD);
      } else {
        gen_operation(REG_MEM, REG_RDI, get_type_size(node->type), OP_SUB);
      }
      compile_node(node->lhs);
      return;
    }
    case ND_SUFFIX_INC:
    case ND_SUFFIX_DEC: {
      compile_node(node->lhs);
      gen_push(REG_RAX);
      gen_assignable_address(node->lhs);
      printf("  mov rdi, 1\n");
      if (node->kind == ND_SUFFIX_INC) {
        gen_operation(REG_MEM, REG_RDI, get_type_size(node->type), OP_ADD);
      } else {
        gen_operation(REG_MEM, REG_RDI, get_type_size(node->type), OP_SUB);
      }
      gen_pop(REG_RAX);
      return;
    }
    case ND_BITWISENOT: {
      compile_node(node->lhs);
      gen_operation(REG_RAX, REG_RAX, get_type_size(node->type), OP_BITWISE_NOT);
      return;
    }
    case ND_LOGICALNOT: {
      compile_node(node->lhs);
      printf("  mov rdi, 1\n");
      gen_operation(REG_RAX, REG_RDI, 8, OP_BITWISE_XOR);
      return;
    }
    case ND_SIZEOF: {
      printf("  mov rax, %d\n", node->type->var_size);
      return;
    }
    default:
      break;
  }

  if (node->kind == ND_FUNCCALL) {
    char *name = calloc(node->func->name_len + 1, sizeof(char));
    memcpy(name, node->func->name, node->func->name_len);
    int arg_count = 0;
    for (Node *now_arg = node->func->args; now_arg != NULL; now_arg = now_arg->next_stmt) {
      compile_node(now_arg);
      gen_push(REG_RAX);
      arg_count++;
    }

    for (int arg_idx = 0; arg_idx < arg_count && arg_idx < 6; arg_idx++) {
      gen_pop(args_reg[arg_idx]);
    }
    printf("  call %s\n", name);
    if (arg_count > 6) {
      gen_emptypop(arg_count - 6);
    }
    return;
  }

  compile_node(node->lhs);
  gen_push(REG_RAX);
  compile_node(node->rhs);
  gen_push(REG_RAX);

  gen_pop(REG_RDI);
  gen_pop(REG_RAX);

  if (node->type->kind >= TY_PTR) {
    printf("  imul rdi, %d\n", node->type->content->var_size);
  }

  int reg_size = get_type_size(node->type);
  int min_reg_size = 8;
  if (node->lhs->type != NULL && min_reg_size > get_type_size(node->lhs->type)) {
    min_reg_size = get_type_size(node->lhs->type);
  }
  if (node->rhs->type != NULL && min_reg_size > get_type_size(node->rhs->type)) {
    min_reg_size = get_type_size(node->rhs->type);
  }


  // calculation
  switch (node->kind) {
    case ND_ADD:
      gen_operation(REG_RAX, REG_RDI, reg_size, OP_ADD);
      break;
    case ND_SUB:
      gen_operation(REG_RAX, REG_RDI, reg_size, OP_SUB);
      break;
    case ND_MUL:
      gen_operation(REG_RAX, REG_RDI, reg_size, OP_MUL);
      break;
    case ND_DIV:
      gen_operation(REG_RAX, REG_RDI, min_reg_size, OP_DIV);
      break;
    case ND_REMAINDER:
      gen_operation(REG_RAX, REG_RDI, min_reg_size, OP_REMAINDER);
      break;
    case ND_LEFTSHIFT:
      gen_operation(REG_RAX, REG_RDI, reg_size, OP_LEFT_SHIFT);
      break;
    case ND_RIGHTSHIFT:
      gen_operation(REG_RAX, REG_RDI, reg_size, OP_RIGHT_SHIFT);
      break;
    case ND_BITWISEAND:
      gen_operation(REG_RAX, REG_RDI, reg_size, OP_BITWISE_AND);
      break;
    case ND_BITWISEXOR:
      gen_operation(REG_RAX, REG_RDI, reg_size, OP_BITWISE_XOR);
      break;
    case ND_BITWISEOR:
      gen_operation(REG_RAX, REG_RDI, reg_size, OP_BITWISE_OR);
      break;
    case ND_EQ:
      gen_compare("sete", min_reg_size);
      break;
    case ND_NEQ:
      gen_compare("setne", min_reg_size);
      break;
    case ND_LC:
      gen_compare("setl", min_reg_size);
      break;
    case ND_LEC:
      gen_compare("setle", min_reg_size);
      break;
    case ND_RC:
      gen_compare("setg", min_reg_size);
      break;
    case ND_REC:
      gen_compare("setge", min_reg_size);
      break;
    default:
      break;
  }
}

void gen_global_var_define(Obj *var) {
  char *global_var_name = calloc(var->name_len+ 1, sizeof(char));
  memcpy(global_var_name, var->name, var->name_len);
  printf(".data\n");
  if (var->type->kind == TY_STR) {
    printf(".LC%d:\n", var->offset);
    printf("  .string \"%s\"\n", var->name);
    return;
  }
  printf("%s:\n", global_var_name);
  switch (var->type->kind) {
    case TY_CHAR:
      printf("  .zero 1\n");
      break;
    case TY_SHORT:
      printf("  .zero 2\n");
      break;
    case TY_INT:
      printf("  .zero 4\n");
      break;
    case TY_LONG:
    case TY_PTR:
    case TY_ARRAY:
      printf("  .zero %d\n", var->type->var_size);
      break;
    default:
      return;
  }
}

void codegen(Function *head_func) {
  printf(".intel_syntax noprefix\n");
  for (Obj *gvar = gvars; gvar != NULL; gvar = gvar->next) {
    gen_global_var_define(gvar);
  }

  for (Function *now_func = head_func; now_func; now_func = now_func->next) {
    char *func_name = calloc(now_func->func_name_len + 1, sizeof(char));
    memcpy(func_name, now_func->func_name, now_func->func_name_len);
    printf(".global %s\n", func_name);
    printf(".text\n");
    printf("%s:\n", func_name);

    // Prologue
    // Allocate variable size.
    gen_push(REG_RBP);
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", now_func->vars_size);

    // Set arguments (use register)
    int arg_count = now_func->func_argc - 1;
    for (Node *arg = now_func->func_args; arg != NULL; arg = arg->lhs) {
      if (arg_count < 6) {
        gen_var_address(arg);
        gen_operation(REG_MEM, args_reg[arg_count], get_type_size(arg->use_var->type), OP_MOV);
      }
      arg_count--;
    }

    // Set arguements (use stack due more than 7 arguments)
    arg_count = now_func->func_argc - 1;
    for (Node *arg = now_func->func_args; arg != NULL; arg = arg->lhs) {
      if (arg_count >= 6) {
        gen_var_address(arg);
        gen_push(REG_RAX);
        printf("  mov rax, QWORD PTR [rbp + %d]\n", 8 + (arg_count - 5) * 8);
        printf("  mov rdi, rax\n");
        gen_pop(REG_RAX);
        gen_operation(REG_MEM, REG_RDI, get_type_size(arg->use_var->type), OP_MOV);
      }
      arg_count--;
    }

    compile_node(now_func->stmt);

    printf("  mov rsp, rbp\n");
    gen_pop(REG_RBP);
    printf("  ret \n");
  }
}
