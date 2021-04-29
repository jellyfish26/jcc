#include "write.h"
#include <stdio.h>

const char *reg_8byte[] = {
  "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
  "QWORD PTR [rax]"
};

const char *reg_4byte[] = {
  "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
  "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
  "DWORD PTR [rax]"
};

const char *reg_2byte[] = {
  "ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
  "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
  "WORD PTR [rax]"
};

const char *reg_1byte[] = {
  "al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl",
  "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
  "BYTE PTR [rax]"
};

const char *get_reg(RegKind reg_kind, RegSizeKind reg_size) {
  switch (reg_size) {
    case REG_SIZE_1:
      return reg_1byte[reg_kind];
    case REG_SIZE_2:
      return reg_2byte[reg_kind];
    case REG_SIZE_4:
      return reg_4byte[reg_kind];
    case REG_SIZE_8:
      return reg_8byte[reg_kind];
  };
}

RegSizeKind convert_type_to_size(TypeKind var_type) {
  switch (var_type) {
    case TY_INT:
      return REG_SIZE_4;
    default:
      return REG_SIZE_8;
  };
}

void gen_compare(char *comp_op, TypeKind type_kind) {
  printf("  cmp %s, %s\n", 
      get_reg(REG_RAX, convert_type_to_size(type_kind)),
      get_reg(REG_RDI, convert_type_to_size(type_kind)));
  printf("  %s al\n", comp_op);
  printf("  movzx rax, al\n");
}

void gen_var_address(Node *node) {
  if (node->kind != ND_VAR && node->kind != ND_ADDR) {
    errorf(ER_COMPILE, "Not variable");
  }

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->var->offset);
  printf("  push rax\n");
}

// left_reg = right_reg
bool gen_instruction_mov(RegKind left_reg, RegKind right_reg, RegSizeKind reg_size) {
  if (left_reg == REG_MEM && right_reg == REG_MEM) {
    return false;
  }
  printf("  mov %s, %s\n",
      get_reg(left_reg, reg_size),
      get_reg(right_reg, reg_size));
  return true;
}

// left_reg = left_reg + right_reg
bool gen_instruction_add(RegKind left_reg, RegKind right_reg, RegSizeKind reg_size) {
  if (left_reg == REG_MEM) {
    return false;
  }
  printf("  add %s, %s\n",
      get_reg(left_reg, reg_size),
      get_reg(right_reg, reg_size));
  return true;
}

