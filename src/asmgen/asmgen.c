#include "asmgen/asmgen.h"

#include <stdarg.h>
#include <stdio.h>

static FILE *target_file;

static void println(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(target_file, fmt, ap);
  fprintf(target_file, "\n");
  va_end(ap);
}

static void gen_push(const char *reg) {
  println("  push %%%s", reg);
}

static void gen_pop(const char *reg) {
  println("  pop %%%s", reg);
}

static void gen_expr(Node *node) {
  if (node->kind == ND_NUM) {
    println("  mov $%ld, %%rax", node->val);
    return;
  }

  gen_expr(node->lhs);
  gen_push("rax");

  gen_expr(node->rhs);
  println("  mov %%rax, %%rdi");
  gen_pop("rax");

  switch (node->kind) {
  case ND_ADD:
    println("  add %%rdi, %%rax");
    break;
  case ND_SUB:
    println("  sub %%rdi, %%rax");
    break;
  case ND_MUL:
    println("  imul %%rdi, %%rax");
    break;
  case ND_DIV:
  case ND_MOD:
    println("  cqo");
    println("  idiv %%rdi");

    if (node->kind == ND_MOD) {
      println("  mov %%rdx, %%rax");
    }
    break;
  case ND_LSHIFT:
    println("  mov %%rdi, %%rcx");
    println("  sal %%cl, %%rax");
    break;
  case ND_RSHIFT:
    println("  mov %%rdi, %%rcx");
    println("  sar %%cl, %%rax");
    break;
  case ND_LCMP:
  case ND_LECMP:
  case ND_EQ:
  case ND_NEQ:
    println("  cmp %%rdi, %%rax");

    if (node->kind == ND_LCMP) {
      println("  setl %%al");
    } else if (node->kind == ND_LECMP) {
      println("  setle %%al");
    } else if (node->kind == ND_EQ) {
      println("  sete %%al");
    } else {
      println("  setne %%al");
    }

    println("  movzx %%al, %%rax");
    break;
  case ND_BITAND:
    println("  and %%rdi, %%rax");
    break;
  case ND_BITXOR:
    println("  xor %%rdi, %%rax");
    break;
  default:
    break;
  }
}

void asmgen(Node *node, char *filename) {
  target_file = fopen(filename, "w");

  println(".globl main");
  println(".text");
  println(".type main, @function");
  println("main:");

  // Prologue
  gen_push("rbp");
  println("  mov %%rsp, %%rbp");

  gen_expr(node);
  
  println("  mov %%rbp, %%rsp");
  gen_pop("rbp");
  println("  ret");
  
  fclose(target_file);
}
