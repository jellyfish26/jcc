#pragma once
#include "../parser/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef enum {
  REG_RAX,  // rax, eax,  ax,   al
  REG_RBX,  // rbx, ebx,  bx,   bl
  REG_RCX,  // rcx, ecx,  cx,   cl
  REG_RDX,  // rdx, edx,  dx,   dl
  REG_RSI,  // rsi, esi,  si,   sil
  REG_RDI,  // rdi, edi,  di,   dil
  REG_RBP,  // rbp, ebp,  bp,   bpl
  REG_RSP,  // rsp, esp,  sp,   spl
  REG_R8,   // r8,  r8d,  r8w,  r8b
  REG_R9,   // r9,  r9d,  r9w,  r9b
  REG_R10,  // r10, r10d, r10w, r10b
  REG_R11,  // r11, r11d, r11w, r11b
  REG_R12,  // r12, r12d, r12w, r12b
  REG_R13,  // r13, r13d, r13w, r13b
  REG_R14,  // r14, r14d, r14w, r14b
  REG_R15   // r15, r15d, r15w, r15b
} RegKind;

typedef enum {
  REG_SIZE_8,  // 8-byte register (64bit)
  REG_SIZE_4,  // 4-byte register (32bit)
  REG_SIZE_2,  // 2-byte register (16bit)
  REG_SIZE_1   // 1-byte register (8bit)
} RegSizeKind;

//
// assembly.c
//

const char *get_reg(RegKind reg_kind, RegSizeKind reg_size);

void gen_compare(char *comp, TypeKind var_type_kind);
void gen_var(Node *node);


//
// codegen.c
//

void codegen();

