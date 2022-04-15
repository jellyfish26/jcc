#include "code/codegen.h"
#include "parser/parser.h"
#include "token/tokenize.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *output_file;

static void println(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  fprintf(output_file, "\n");
  va_end(ap);
}

//
// About assembly
//

void compile_node(Node *node);

static void gen_push(const char *reg) {
  println("  push %%%s", reg);
}

static void gen_pop(const char *reg) {
  println("  pop %%%s", reg);
}

static void gen_emptypop(int num) {
  println("  add $%d, %%rsp", num * 8);
}

// Compute the address of a given node.
// In the case of a local variable, it computes the relative address to the base pointer,
// and stores the absolute address in the RAX register.
static void gen_addr(Node *node) {
  switch (node->kind) {
    case ND_VAR:
      if (node->var->is_global) {
        println("  mov $%s, %%rax", node->var->name);
        return;
      }

      if (node->var->ty->kind == TY_VLA && !node->var->is_allocate) {
        compile_node(node->var->ty->vla_size);
        println("  mov %%rsp, %%rsi");
        println("  mov %%rbp, %%rcx");
        println("  sub %%rsp, %%rcx");
        println("  sub -8(%%rbp), %%rcx");
        println("  add %%rax, -8(%%rbp)");
        println("  sub %%rax, %%rsp");
        println("  mov %%rsp, %%rdi");
        println("  rep movsb");
        println("  mov %%rdi, -%d(%%rbp)", node->var->offset);
        node->var->is_allocate = true;
      }

      println("  mov %%rbp, %%rax");
      println("  sub $%d, %%rax", node->var->offset);

      if (node->var->ty->kind == TY_VLA) {
        println("  mov (%%rax), %%rax");
      }
      return;
    case ND_CONTENT:
      compile_node(node->lhs);
      return;
    case ND_FUNCCALL:
      compile_node(node);
      return;
    default:
      printf("%d\n", node->kind);
      errorf_tkn(ER_COMPILE, node->tkn, "Not a variable.");
  }
}

static void gen_load(Type *ty) {
  if (ty->kind == TY_ARRAY || ty->kind == TY_VLA || is_struct_type(ty)) {
    // If the type is array, string literal, struct, or union,
    // it will automatically be treated as a pointer
    // and we cannot load the content direclty.
    return;
  }

  if (ty->kind == TY_FLOAT) {
    println("  movss (%%rax), %%xmm0");
    return;
  } else if (ty->kind == TY_DOUBLE) {
    println("  movsd (%%rax), %%xmm0");
    return;
  } else if (ty->kind == TY_LDOUBLE) {
    println("  fldt (%%rax)");
    return;
  }

  char *unsi = ty->is_unsigned ? "movzx" : "movsx";

  // When char and short size values are loaded with mov instructions,
  // they may contain garbage in the lower 32bits,
  // so we are always extended to int.
  if (ty->var_size == 1) {
    println("  %sb (%%rax), %%eax", unsi);
  } else if (ty->var_size == 2) {
    println("  %sw (%%rax), %%eax", unsi);
  } else if (ty->var_size == 4) {
    println("  mov (%%rax), %%eax");
  } else {
    println("  mov (%%rax), %%rax");
  }

  if (ty->bit_field > 0) {
    println("  shl $%d, %%rax", 64 - ty->bit_offset - ty->bit_field);
    println("  %s $%d, %%rax", ty->is_unsigned ? "shr" : "sar", 64 - ty->bit_field);
  }
}

// Store the value of the rax register at the address pointed to by the top of the stack.
static void gen_store(Type *ty) {
  gen_pop("rdi");

  if (ty->kind == TY_FLOAT) {
    println("  movss %%xmm0, (%%rdi)");
    return;
  } else if (ty->kind == TY_DOUBLE) {
    println("  movsd %%xmm0, (%%rdi)");
    return;
  } else if (ty->kind == TY_LDOUBLE) {
    println("  fstpt (%%rdi)");
    return;
  }

  if (ty->bit_field > 0) {
    gen_push("rdi");
    gen_push("rdx");
    println("  movabs $%ld, %%rdx", (1LL<<ty->bit_field) - 1);
    println("  and %%rdx, %%rax");
    println("  shl $%d, %%rax", ty->bit_offset);
    println("  mov (%%rdi), %%rdi");
    println("  movabs $%ld, %%rdx", (-1LL) - (((1LL<<ty->bit_field)- 1)<<ty->bit_offset));
    println("  and %%rdx, %%rdi");
    println("  or %%rdi, %%rax");
    gen_pop("rdx");
    gen_pop("rdi");
  }

  if (ty->var_size == 1) {
    println("  mov %%al, (%%rdi)");
  } else if (ty->var_size == 2) {
    println("  mov %%ax, (%%rdi)");
  } else if (ty->var_size == 4) {
    println("  mov %%eax, (%%rdi)");
  } else {
    println("  mov %%rax, (%%rdi)");
  }
}

static char *argregs8[] =  {"%dil", "%sil", "%dl",  "%cl",  "%r8b", "%r9b"};
static char *argregs16[] = {"%di",  "%si",  "%dx",  "%cx",  "%r8w", "%r9w"};
static char *argregs32[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
static char *argregs64[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8",  "%r9"};

static char i32i8[]  = "movsx %al, %eax";
static char i32u8[]  = "movzx %al, %eax";
static char i32i16[] = "movsx %ax, %eax";
static char i32u16[] = "movzx %ax, %eax";
static char i64i32[] = "movsxd %eax, %rax";
static char i64u32[] = "mov %eax, %eax";

static char i8f32[]  = "cvttss2si %xmm0, %eax\n  movsx %al, %eax";
static char u8f32[]  = "cvttss2si %xmm0, %eax\n  movzx %al, %eax";
static char i16f32[] = "cvttss2si %xmm0, %eax\n  movsx %ax, %eax";
static char u16f32[] = "cvttss2si %xmm0, %eax\n  movzx %ax, %eax";
static char i32f32[] = "cvttss2si %xmm0, %eax";
static char u32f32[] = "cvttss2si %xmm0, %rax\n  mov %eax, %eax";
static char i64f32[] = "cvttss2si %xmm0, %rax";
static char u64f32[] = 
    "sub $8, %rsp\n"
  "  movss %xmm0, 4(%rsp)\n"
  "  movl $1593835520, (%rsp)\n"
  "  comiss (%rsp), %xmm0\n"
  "  jnb 1f\n"
  "  comiss 4(%rsp), %xmm0\n"
  "  cvttss2si %xmm0, %rax\n"
  "  jmp 2f\n"
  "1:\n"
  "  movss 4(%rsp), %xmm0\n"
  "  movss (%rsp), %xmm1\n"
  "  subss %xmm1, %xmm0\n"
  "  cvttss2si %xmm0, %rax\n"
  "  movabs $-9223372036854775808, %rdx\n"
  "  xor %rdx, %rax\n"
  "2:\n"
  "  add $8, %rsp";

static char f32i8[]  = "movsx %al, %eax\n  cvtsi2ss %eax, %xmm0";
static char f32u8[]  = "movzx %al, %eax\n  cvtsi2ss %eax, %xmm0";
static char f32i16[] = "movsx %ax, %eax\n  cvtsi2ss %eax, %xmm0";
static char f32u16[] = "movzx %ax, %eax\n  cvtsi2ss %eax, %xmm0";
static char f32i32[] = "cvtsi2ss %eax, %xmm0";
static char f32u32[] = "mov %eax, %eax\n  cvtsi2ss %rax, %xmm0";
static char f32i64[] = "cvtsi2ss %rax, %xmm0";
static char f32u64[] = 
    "test %rax, %rax\n"
  "  pxor %xmm0, %xmm0\n"
  "  js 1f\n"
  "  cvtsi2ss %rax, %xmm0\n"
  "  jmp 2f\n"
  "1:\n"
  "  mov %rax, %rdx\n"
  "  shr %rdx\n"
  "  and $1, %eax\n"
  "  or %rax, %rdx\n"
  "  cvtsi2ss %rdx, %xmm0\n"
  "  addss %xmm0, %xmm0\n"
  "2:";

static char i8f64[]  = "cvttsd2si %xmm0, %eax\n  movsx %al, %eax";
static char u8f64[]  = "cvttsd2si %xmm0, %eax\n  movzx %al, %eax";
static char i16f64[] = "cvttsd2si %xmm0, %eax\n  movsx %ax, %eax";
static char u16f64[] = "cvttsd2si %xmm0, %eax\n  movzx %ax, %eax";
static char i32f64[] = "cvttsd2si %xmm0, %eax";
static char u32f64[] = "cvttsd2si %xmm0, %rax\n  mov %eax, %eax";
static char i64f64[] = "cvttsd2si %xmm0, %rax";
static char u64f64[] = 
    "sub $16, %rsp\n"
  "  movsd %xmm0, 8(%rsp)\n"
  "  movl $0, (%rsp)\n"
  "  movl $1138753536, 4(%rsp)\n"
  "  comisd (%rsp), %xmm0\n"
  "  jnb 1f\n"
  "  movsd 8(%rsp), %xmm0\n"
  "  cvttsd2si %xmm0, %rax\n"
  "  jmp 2f\n"
  "1:\n"
  "  movsd 8(%rsp), %xmm0\n"
  "  movsd (%rsp), %xmm1\n"
  "  subsd %xmm1, %xmm0\n"
  "  cvttsd2si %xmm0, %rax\n"
  "  movabs $-9223372036854775808, %rdx\n"
  "  xor %rdx, %rax\n"
  "2:\n"
  "  add $16, %rsp";

static char f32f64[] = "cvtsd2ss %xmm0, %xmm0";

static char f64i8[]  = "movsx %al, %eax\n  cvtsi2sd %eax, %xmm0";
static char f64u8[]  = "movzx %al, %eax\n  cvtsi2sd %eax, %xmm0";
static char f64i16[] = "movsx %ax, %eax\n  cvtsi2sd %eax, %xmm0";
static char f64u16[] = "movzx %ax, %eax\n  cvtsi2sd %eax, %xmm0";
static char f64i32[] = "cvtsi2sd %eax, %xmm0";
static char f64u32[] = "mov %eax, %eax\n  cvtsi2sd %rax, %xmm0";
static char f64i64[] = "cvtsi2sd %rax, %xmm0";
static char f64u64[] = 
    "test %rax, %rax\n"
  "  pxor %xmm0, %xmm0\n"
  "  js 1f\n"
  "  cvtsi2sd %rax, %xmm0\n"
  "  jmp 2f\n"
  "1:\n"
  "  mov %rax, %rdx\n"
  "  shr %rdx\n"
  "  and $1, %eax\n"
  "  or %rax, %rdx\n"
  "  cvtsi2sd %rdx, %xmm0\n"
  "  addsd %xmm0, %xmm0\n"
  "2:";
static char f64f32[] = "cvtss2sd %xmm0, %xmm0";

#define F80_TO_INT_1 \
    "sub $16, %rsp\n" \
  "  fnstcw 12(%rsp)\n" \
  "  movzwl 12(%rsp), %eax\n" \
  "  orb $12, %ah\n" \
  "  mov %ax, 8(%rsp)\n" \
  "  fldcw 8(%rsp)\n"

#define F80_TO_INT_2 \
  "  fldcw 12(%rsp)\n"

#define F80_TO_INT_3 \
  "  add $16, %rsp"

static char i8f80[]  = F80_TO_INT_1 "  fistpq (%rsp)\n" F80_TO_INT_2 "  mov (%rsp), %al\n  movsx %al, %eax\n" F80_TO_INT_3;
static char u8f80[]  = F80_TO_INT_1 "  fistpq (%rsp)\n" F80_TO_INT_2 "  movzxb (%rsp), %eax\n" F80_TO_INT_3;
static char i16f80[] = F80_TO_INT_1 "  fistpq (%rsp)\n" F80_TO_INT_2 "  xor %rax, %rax\n  mov (%rsp), %ax\n" F80_TO_INT_3;
static char u16f80[] = F80_TO_INT_1 "  fistpq (%rsp)\n" F80_TO_INT_2 "  movzxw (%rsp), %eax\n" F80_TO_INT_3;
static char i32f80[] = F80_TO_INT_1 "  fistpq (%rsp)\n" F80_TO_INT_2 "  mov (%rsp), %eax\n" F80_TO_INT_3;
static char u32f80[] = F80_TO_INT_1 "  fistpq (%rsp)\n" F80_TO_INT_2 "  movzxl (%rsp), %rax\n" F80_TO_INT_3;
static char i64f80[] = F80_TO_INT_1 "  fistpq (%rsp)\n" F80_TO_INT_2 "  mov (%rsp), %rax\n" F80_TO_INT_3;

static char u64f80[] =
    "sub $32, %rsp\n"
  "  fstpt 16(%rsp)\n"
  "  movl $0, (%rsp)\n"
  "  movl $-2147483648, 4(%rsp)\n"
  "  movl $16446, 8(%rsp)\n"
  "  movl $0, 12(%rsp)\n"
  "  fldt (%rsp)\n"
  "  fldt 16(%rsp)\n"
  "  fcomip %st(1), %st\n"
  "  fstp %st(0)\n"
  "  jnb 1f\n"
  "  fldt 16(%rsp)\n"
  "  " F80_TO_INT_1
  "  fistpq (%rsp)\n"
  F80_TO_INT_2
  "  mov (%rsp), %rax\n"
  F80_TO_INT_3 "\n"
  "  jmp 2f\n"
  "1:"
  "  fldt 16(%rsp)\n"
  "  fldt (%rsp)\n"
  "  fsubrp %st, %st(1)\n"
  "  " F80_TO_INT_1
  "  fistpq (%rsp)\n"
  F80_TO_INT_2
  "  mov (%rsp), %rax\n"
  F80_TO_INT_3 "\n"
  "  movabs $-9223372036854775808, %rdi\n"
  "  xor %rdi, %rax\n"
  "2:\n"
  "  add $32, %rsp";

static char f32f80[] = 
    "sub $4, %rsp\n"
  "  fstps 4(%rsp)\n"
  "  movss 4(%rsp), %xmm0\n"
  "  add $8, %rsp\n";

static char f64f80[] = 
    "sub $8, %rsp\n"
  "  fstpl 8(%rsp)\n"
  "  movsd 8(%rsp), %xmm0\n"
  "  add $8, %rsp\n";

static char f80i8[]  = "sub $4, %rsp\n  movsx %al, %eax\n  mov %eax, (%rsp)\n  fildl (%rsp)\n  add $4, %rsp";
static char f80u8[]  = "sub $4, %rsp\n  movzx %al, %eax\n  mov %eax, (%rsp)\n  fildl (%rsp)\n  add $4, %rsp";
static char f80i16[] = "sub $4, %rsp\n  mov %ax, (%rsp)\n  fildl (%rsp)\n  add $4, %rsp";
static char f80u16[] = "sub $4, %rsp\n  movzxl %ax, (%rsp)\n  fildl (%rsp)\n  add $4, %rsp";
static char f80i32[] = "sub $4, %rsp\n  mov %eax, (%rsp)\n  fildl (%rsp)\n  add $4, %rsp";
static char f80u32[] = "sub $8, %rsp\n  movzxq %eax, (%rsp)\n  fildq (%rsp)\n  add $8, %rsp";
static char f80i64[] = "sub $8, %rsp\n  mov %rax, (%rsp)\n  fildq (%rsp)\n  add $8, %rsp";
static char f80f32[] = "sub $4, %rsp\n  movss %xmm0, (%rsp)\n  flds (%rsp)\n  add $4, %rsp";
static char f80f64[] = "sub $8, %rsp\n  movsd %xmm0, (%rsp)\n  fldl (%rsp)\n  add $8, %rsp";

static char f80u64[] =
    "sub $32, %rsp\n"
  "  movl $0, (%rsp)\n"
  "  movl $-2147483648, 4(%rsp)\n"
  "  movl $16447, 8(%rsp)\n"
  "  movl $0, 12(%rsp)\n"
  "  mov %rax, 16(%rsp)\n"
  "  fildq 16(%rsp)\n"
  "  cmpq $0, 16(%rsp)\n"
  "  jns 1f\n"
  "  fldt (%rsp)\n"
  "  faddp %st, %st(1)\n"
  "1:\n"
  "  add $32, %rsp";

static char *cast_table[][11] = {
// i8     i16     i32     i64     u8     u16     u32     u64     f32     f64     f80    to/from
  {NULL,  NULL,   NULL,   i64i32, i32u8, i32u16, NULL,   i64i32, f32i8,  f64i8,  f80i8},  // i8
  {i32i8, NULL,   NULL,   i64i32, i32u8, i32u16, NULL,   i64i32, f32i16, f64i16, f80i16}, // i16
  {i32i8, i32i16, NULL,   i64i32, i32u8, i32u16, NULL,   i64i32, f32i32, f64i32, f80i32}, // i32
  {i32i8, i32i16, NULL,   NULL,   i32u8, i32u16, NULL,   NULL,   f32i64, f64i64, f80i64}, // i64

  {i32i8, NULL,   NULL,   i64i32, i32u8, i32u16, NULL,   i64i32, f32u8,  f64u8,  f80u8},  // u8
  {i32i8, i32i16, NULL,   i64i32, i32u8, NULL,   NULL,   i64i32, f32u16, f64u16, f80u16}, // u16
  {i32i8, i32i16, NULL,   i64u32, i32u8, i32u16, NULL,   i64u32, f32u32, f64u32, f80u32}, // u32
  {i32i8, i32i16, NULL,   NULL,   i32u8, i32u16, NULL,   NULL,   f32u64, f64u64, f80u64}, // u64

  {i8f32, i16f32, i32f32, i64f32, u8f32, u16f32, u32f32, u64f32, NULL,   f64f32, f80f32}, // f32
  {i8f64, i16f64, i32f64, i64f64, u8f64, u16f64, u32f64, u64f64, f32f64, NULL,   f80f64}, // f64
  {i8f80, i16f80, i32f80, i64f80, u8f80, u16f80, u32f80, u64f80, f32f80, f64f80, NULL},   // f80
};

static int get_type_idx(Type *ty) {
  ty = extract_type(ty);

  int ret;
  switch (ty->kind) {
    case TY_CHAR:
      ret = 0;
      break;
    case TY_SHORT:
      ret = 1;
      break;
    case TY_INT:
      ret = 2;
      break;
    case TY_LONG:
    case TY_PTR:
    case TY_ARRAY:
    case TY_VLA:
      ret = 3;
      break;
    case TY_FLOAT:
      ret = 8;
      break;
    case TY_DOUBLE:
      ret = 9;
      break;
    case TY_LDOUBLE:
      ret = 10;
      break;
    default:
      return 0;
  };
  if (ty->is_unsigned) {
    ret += 4;
  }
  return ret;
}

static void gen_cast(Node *node) {
  compile_node(node->lhs);
  int from = get_type_idx(node->lhs->ty);
  int to = get_type_idx(node->ty);
  if (cast_table[from][to] != NULL) {
    println("  %s", cast_table[from][to]);
  }
}

int branch_label = 0;

void expand_ternary(Node *node, int label) {
  compile_node(node->cond);
  println("  cmp $0, %%rax");
  println("  je .Lfalse%d", label);

  compile_node(node->lhs);
  println("  jmp .Lnext%d", label);
  println(".Lfalse%d:", label);

  compile_node(node->rhs);
  println(".Lnext%d:", label);
}

static void gen_lvar_init(Node *node) {
  gen_addr(node->lhs);

  // zero fill
  gen_push("rax");
  println("  mov %%rax, %%rdi");
  println("  xor %%rax, %%rax");
  println("  mov $%d, %%rcx", node->lhs->ty->var_size);
  println("  rep stosb");
  gen_pop("rax");

  int bytes = 0, bit_offset = 0;
  for (Node *expr = node->rhs; expr != NULL; expr = expr->lhs) {
    gen_push("rax");
    if (expr->ty->bit_field > 0) {
      if (bit_offset + expr->ty->bit_field > expr->ty->var_size * 8) {
        println("  addq $%d, (%%rsp)", align_to(bit_offset, 8) / 8);
        bytes += align_to(bit_offset, 8) / 8;
        bit_offset = 0;
      }
      bit_offset += expr->ty->bit_field;

      if (expr->init != NULL) {
        compile_node(expr->init);
      } else {
        println("  xor %%rax, %%rax");
      }

      gen_store(expr->ty);
      println("  mov %%rdi, %%rax");
      continue;
    }

    if (align_to(bit_offset, 8) != 0) {
      println("  addq $%d, (%%rsp)", align_to(bit_offset, 8) / 8);
      bytes += align_to(bit_offset, 8) / 8;
      bit_offset = 0;
    }

    int padding = align_to(bytes, expr->ty->align) - bytes;
    if (padding != 0) {
      println("  addq $%d, (%%rsp)", padding);
      bytes += padding;
    }

    if (expr->init != NULL) {
      switch (extract_type(expr->init->ty)->kind) {
        case TY_STRUCT:
        case TY_UNION:
          if (expr->init->kind == ND_ASSIGN) {
            compile_node(expr->init);
            gen_addr(expr->init->lhs);
          } else {
            compile_node(expr->init);
          }
          println("  mov %%rax, %%rsi");
          gen_pop("rdi");
          println("  mov $%d, %%rcx", expr->ty->var_size);
          println("  rep movsb");
          println("  mov %%rdi, %%rax");
          break;
        default:
          compile_node(expr->init);
          gen_store(expr->ty);
          println("  mov %%rdi, %%rax");
          println("  add $%d, %%rax", expr->ty->var_size);
      }
    } else {
      gen_pop("rdi");
      println("  xor %%rax, %%rax");
      println("  mov $%d, %%rcx", expr->ty->var_size);
      println("  rep stosb");
      println("  mov %%rdi, %%rax");
    }
    bytes += expr->ty->var_size;
  }
}

static void gen_gvar_init(Node *node) {
  Obj *obj = node->kind == ND_INIT ? node->lhs->var : node->var;
  println(".data");
  println("%s:", obj->name);

  if (node->kind == ND_VAR) {
    println("  .zero %d", obj->ty->var_size);
    return;
  }

  int bytes = 0, bit_offset = 0;
  int64_t *bit = calloc(1, sizeof(int64_t));
  for (Node *expr = node->rhs; expr != NULL; expr = expr->lhs) {
    if (expr->ty->bit_field > 0) {
      if (bit_offset + expr->ty->bit_field > expr->ty->var_size * 8) {
        for (int i = 0; i < align_to(bit_offset, 8) / 8; i++) {
          println("  .byte %d", *((int8_t *)bit + i));
        }
        bytes += align_to(bit_offset, 8) / 8;
        bit_offset = 0;
        *bit = 0;
      }

      int64_t val = expr->init->val;
      val &= ((1LL<<expr->ty->bit_field) - 1);
      val <<= expr->ty->bit_offset;
      *bit |= val;
      bit_offset += expr->ty->bit_field;
      continue;
    }

    for (int i = 0; i < align_to(bit_offset, 8) / 8; i++) {
      println("  .byte %d", *((int8_t *)bit + i));
    }
    bytes += align_to(bit_offset, 8) / 8;
    bit_offset = 0;
    *bit = 0;

    int padding = align_to(bytes, expr->ty->align) - bytes;
    if (padding != 0) {
      println("  .zero %d", padding);
      bytes += padding;
    }
    bytes += expr->ty->var_size;

    if (expr->init == NULL) {
      println("  .zero %d", expr->ty->var_size);
      continue;
    }

    if (expr->ty->kind == TY_FLOAT) {
      float *ptr = calloc(1, sizeof(float));
      *ptr = (float)expr->init->fval;
      println("  .long %d", *(int*)ptr);
      free(ptr);
      continue;
    }

    if (expr->ty->kind == TY_DOUBLE) {
      double *ptr = calloc(1, sizeof(double));
      *ptr = (double)expr->init->fval;
      println("  .long %d", *(int*)ptr);
      println("  .long %d", *((int*)ptr + 1));
      free(ptr);
      continue;
    }

    if (expr->ty->kind == TY_LDOUBLE) {
      long double *ptr = calloc(1, sizeof(double));
      *ptr = expr->init->fval;
      for (int i = 0; i < 4; i++) {
        println("  .long %d", *((int*)ptr + i));
      }
      free(ptr);
      continue;
    }

    char *asm_ty;
    switch (expr->ty->var_size) {
      case 1:
        asm_ty = ".byte";
        break;
      case 2:
        asm_ty = ".short";
        break;
      case 4:
        asm_ty = ".long";
        break;
      case 8:
        asm_ty = ".quad";
        break;
      default:
        return;
    }

    char *ptr_label = expr->init->var->name;
    if (ptr_label == NULL) {
      println("  %s %d", asm_ty, expr->init->val);
    } else {
      println("  %s %s%+d", asm_ty, ptr_label, expr->init->val);
    }
  }

  for (int i = 0; i < align_to(bit_offset, 8) / 8; i++) {
    println("  .byte %d", *((int8_t *)bit + i));
  }
  bytes += align_to(bit_offset, 8) / 8;
  if (bytes < obj->ty->var_size) {
    println("  .zero %d", obj->ty->var_size - bytes);
  }
}

static bool has_only_float(Type *ty, int begin, int end, int offset) {
  if (ty->kind == TY_STRUCT) {
    for (Member *member = ty->members; member != NULL; member = member->next) {
      if (!has_only_float(member->ty, begin, end, member->offset)) {
        return false;
      }
    }
    return true;
  }

  if (ty->kind == TY_ARRAY) {
    for (int i = 0; i < ty->array_len; i++) {
      if (!has_only_float(ty->base, begin, end, offset + ty->base->var_size * i)) {
        return false;
      }
    }
    return true;
  }

  bool ret = offset < begin || end <= offset;  // Out of bounds is true
  ret |= ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE;
  return ret;
}

// Returns a bool value indicating whether or not the structure type is passing by stack.
//
// When f80_to_register is true, it returns false for the long double type alone.
// Normally, a structure containing a long double type will exchange data through stack,
// but when returning from a function,
// a structure with only one long double type will exchange data through st registers.
static bool is_struct_pass_by_stack(Type *ty, int flcnt, int gecnt, bool f80_to_register) {
  if (ty->var_size == 16 && ty->members->ty->kind == TY_LDOUBLE && f80_to_register) {
    return false;
  }

  if (ty->var_size > 16 || ty->members->ty->kind == TY_LDOUBLE) {
    return true;
  }

  bool f1 = has_only_float(ty, 0, 8, 0);
  bool f2 = has_only_float(ty, 8, 16, 0);

  return flcnt + f1 + f2 >= 8 || gecnt + !f1 + !f2 >= 6;
}

// Recursive
static void push_argsre(Node *node, bool pass_stack) {
  if (node == NULL) {
    return;
  }
  push_argsre(node->next, pass_stack);

  if ((pass_stack && !node->pass_by_stack) || (!pass_stack && node->pass_by_stack)) {
    return;
  }

  Type *ty = extract_type(node->lhs->ty);
  switch (ty->kind) {
    case TY_FLOAT:
    case TY_DOUBLE:
      compile_node(node->lhs);
      println("  movd %%xmm0, %%rax");
      gen_push("rax");
      break;
    case TY_LDOUBLE:
      compile_node(node->lhs);
      println("  sub $16, %%rsp");
      println("  fstpt (%%rsp)");
      break;
    case TY_STRUCT:
    case TY_UNION: {
      println("  sub $%d, %%rsp", align_to(ty->var_size, 8));
      gen_addr(node->lhs);
      println("  mov %%rax, %%rsi");
      println("  mov %%rsp, %%rdi");
      println("  mov $%d, %%rcx", ty->var_size);
      println("  rep movsb");
      break;
    }
    default:
      compile_node(node->lhs);
      gen_push("rax");
  }
}

static void push_args(Node *node, bool pass_stack, int flcnt, int gecnt) {
  for (Node *arg = node->args; arg != NULL; arg = arg->next) {
    Type *arg_ty = extract_type(arg->lhs->ty);

    switch (arg_ty->kind) {
      case TY_FLOAT:
      case TY_DOUBLE:
        if (flcnt >= 8) {
          arg->pass_by_stack = true;
        }
        flcnt++;
        break;
      case TY_LDOUBLE:
        arg->pass_by_stack = true;
        break;
      case TY_STRUCT:
      case TY_UNION: {
        if (is_struct_pass_by_stack(arg_ty, flcnt, gecnt, false)) {
          arg->pass_by_stack = true;
          break;
        }

        bool f1 = has_only_float(arg_ty, 0, 8, 0);
        bool f2 = has_only_float(arg_ty, 8, 16, 0);

        flcnt += f1 + f2;
        gecnt += !f1 + !f2;
        break;
      }
      default:
        if (gecnt >= 6) {
          arg->pass_by_stack = true;
        }
        gecnt++;
    }
  }
  push_argsre(node->args, pass_stack);
}

void compile_node(Node *node) {
  if (node->kind == ND_VOID) {
    return;
  }

  if (node->kind == ND_NUM) {
    TypeKind num_kind = node->ty->kind;

    switch (num_kind) {
      case TY_CHAR:
      case TY_SHORT:
      case TY_INT:
      case TY_LONG:
        println("  movabs $%ld, %%rax", node->val);
        return;
      case TY_FLOAT:
      case TY_DOUBLE:
      case TY_LDOUBLE: {
        void *ptr = calloc(1, 16);
        int sz = 4;

        if (num_kind == TY_FLOAT) {
          *(float*)ptr = (float)node->fval;
          sz = 1;
        } else if (num_kind == TY_DOUBLE) {
          *(double*)ptr = (double)node->fval;
          sz = 2;
        } else {
          *(long double*)ptr = (long double)node->fval;
          sz = 4;
        }

        // insert value to stack
        println("  sub $%d, %%rsp", 4 * sz);
        for (int i = 0; i < sz; i++) {
          println("  movl $%d, %d(%%rsp)", *((int*)ptr + i), 4 * i);
        }

        // load to register (xmm0 or st)
        if (num_kind == TY_FLOAT)
          println("  movss (%%rsp), %%xmm0");
        else if (num_kind == TY_DOUBLE)
          println("  movsd (%%rsp), %%xmm0");
        else
          println("  fldt (%%rsp)");

        println("  add $%d, %%rsp", 4 * sz);
        return;
      }
      default:
        return;
    }
  }

  if (node->kind == ND_CAST) {
    gen_cast(node);
    return;
  } 

  if (node->kind == ND_BLOCK || node->kind == ND_COMMA) {
    for (Node *expr = node->deep; expr != NULL; expr = expr->next) {
      compile_node(expr);
    }
    return;
  }

  if (node->kind == ND_INIT) {
    gen_addr(node->lhs);
    gen_lvar_init(node);
    return;
  }

  switch (node->kind) {
    case ND_VAR:
      if (node->var->ty->kind == TY_ENUM) {
        println("  movabs $%ld, %%rax", node->var->val);
      } else {
        gen_addr(node);
        gen_load(node->ty);
      }
      return;
    case ND_ADDR:
      gen_addr(node->lhs);
      return;
    case ND_ASSIGN: {
      if (is_struct_type(node->ty)) {
        compile_node(node->lhs);
        gen_push("rax");

        if (node->rhs->kind == ND_ASSIGN) {
          gen_addr(node->rhs->lhs);
        } else {
          gen_addr(node->rhs);
        }

        println("  mov %%rax, %%rsi");
        gen_pop("  rdi");
        println("  mov $%d, %%rcx", node->ty->var_size);
        println("  rep movsb");
      } else {
        gen_addr(node->lhs);
        gen_push("rax");
        compile_node(node->rhs);
        gen_store(node->ty);
      }
      return;
    }
    case ND_RETURN:
      if (is_struct_type(node->ty)) {
        gen_addr(node->lhs);
        bool ld1 = is_struct_pass_by_stack(node->ty->ret_ty, 0, 0, true);
        bool ld2 = is_struct_pass_by_stack(node->ty->ret_ty, 0, 0, false);

        // one long double
        if (ld1 != ld2) {
          println("  fldt (%%rax)");
        } else if (ld1) {
          println("  mov %%rax, %%rsi");
          println("  mov -%d(%%rbp), %%rdi", node->ty->var_size + 8);
          println("  mov $%d, %%rcx", node->ty->ret_ty->var_size);
          println("  rep movsb");
          println("  mov -%d(%%rbp), %%rax", node->ty->var_size + 8);
        } else {
          bool f1 = has_only_float(node->ty->ret_ty, 0, 8, 0);
          bool f2 = has_only_float(node->ty->ret_ty, 8, 16, 0);

          int gecnt = 0, flcnt = 0;

          println("  mov %%rax, %%rdi");

          if (f1) {
            println("  movq (%%rdi), %%xmm0");
            flcnt++;
          } else {
            println("  movq (%%rdi), %%rax");
            gecnt++;
          }

          if (node->ty->ret_ty->var_size > 8) {
            if (f2) {
              println("  movq 8(%%rdi), %%xmm%d", flcnt);
            } else {
              println("  movq 8(%%rdi), %s", gecnt == 0 ? "%rax" : "%rdx");
            }
          }
        }
      } else {
        compile_node(node->lhs);
      }
      println("  mov %%rbp, %%rsp");
      gen_pop("rbp");
      println("  ret");
      return;
    case ND_IF: {
      int now_label = branch_label++;
      compile_node(node->cond);
      println("  cmp $0, %rax");
      println("  je .Lelse%d", now_label);

      // "true"
      compile_node(node->then);
      println("  jmp .Lend%d", now_label);

      println(".Lelse%d:", now_label);
      // "else" statement
      if (node->other != NULL) {
        compile_node(node->other);
      }

      // continue
      println(".Lend%d:", now_label);
      return;
    }
    case ND_COND: {
      int now_label = branch_label++;
      expand_ternary(node, branch_label);
      return;
    }
    case ND_FOR: {
      if (node->init != NULL) {
        compile_node(node->init);
      }

      println("%s:", node->conti_label);

      // judege expr
      if (node->cond != NULL) {
        compile_node(node->cond);
        println("  cmp $0, %%rax");
        println("  je %s", node->break_label);
      }

      compile_node(node->then);

      // repeat expr
      if (node->loop != NULL) {
        compile_node(node->loop);
      }

      // finally
      println("  jmp %s", node->conti_label);
      println("%s:", node->break_label);
      return;
    }
    case ND_DO: {
      println("%s:", node->conti_label);
      compile_node(node->then);

      compile_node(node->cond);
      println("  cmp $0, %%rax");
      println("  jne %s", node->conti_label);

      println("%s:", node->break_label);
      return;
    }
    case ND_SWITCH: {
      int now_label = branch_label++;
      compile_node(node->cond);

      int cnt = 0;
      for (Node *expr = node->case_stmt; expr != NULL; expr = expr->case_stmt) {
        println("  cmp $%ld, %%rax", expr->val);
        println("  je .Lcase%d_%d", now_label, cnt++);
      }

      if (node->default_stmt == NULL) {
        println("  jmp %s", node->break_label);
      } else {
        println("  jmp .Ldefault%d", now_label);
      }

      cnt = 0;
      for (Node *expr = node->lhs; expr != NULL; expr = expr->next) {
        if (expr->kind == ND_CASE) {
          println(".Lcase%d_%d:", now_label, cnt++);
        } else if (expr->kind == ND_DEFAULT) {
          println(".Ldefault%d:", now_label);
        }
        compile_node(expr);
      }

      println("%s:", node->break_label);
      return;
    }
    case ND_CASE:
    case ND_DEFAULT:
      compile_node(node->deep);
      return;
    case ND_BREAK:
      println("  jmp %s", node->break_label);
      return;
    case ND_CONTINUE:
      println("  jmp %s", node->conti_label);
      return;
    case ND_CONTENT:
      compile_node(node->lhs);
      gen_load(node->ty);
      return;
    case ND_GOTO:
      println("  jmp %s", node->label);
      return;
    case ND_LABEL:
      println("%s:", node->label);
      return;
    case ND_BITWISENOT:
      compile_node(node->lhs);
      println("  not %%rax");
      return;
    default:
      break;
  }

  if (node->kind == ND_FUNCCALL) {
    Type *ty = node->func->ty;

    // floating cnt, general cnt, stack cnt
    // pass by stack if flcnt >= 9 or gecnt >= 6
    int flcnt = 0, gecnt = 0, stcnt = 0;

    Type *ret_ty = ty->ret_ty;
    if (is_struct_type(ret_ty) && is_struct_pass_by_stack(ret_ty, flcnt, gecnt, true)) {
      println("  sub $%d, %%rsp", align_to(ret_ty->var_size, 8));
      println("  mov %%rsp, %s", argregs64[gecnt++]);
      stcnt += align_to(ret_ty->var_size, 8) / 8;
    }

    push_args(node, true, flcnt, gecnt);
    push_args(node, false, flcnt, gecnt);

    for (Node *arg = node->args; arg != NULL; arg = arg->next) {
      Type *ty = extract_type(arg->lhs->ty);

      switch (ty->kind) {
        case TY_FLOAT:
        case TY_DOUBLE:
          if (flcnt < 8) {
            gen_pop("rax");
            println("  movq %%rax, %%xmm%d", flcnt++);
          } else {
            stcnt++;
          }
          break;
        case TY_LDOUBLE:
          stcnt += 2;
          break;
        case TY_STRUCT:
        case TY_UNION: {
          if (arg->pass_by_stack) {
            stcnt += align_to(ty->var_size, 8) / 8;
            break;
          }

          bool f1 = has_only_float(ty, 0, 8, 0);
          bool f2 = has_only_float(ty, 8, 16, 0);

          gen_pop("rax");
          if (f1) {
            println("  movq %%rax, %%xmm%d", flcnt++);
          } else {
            println("  mov %%rax, %s", argregs64[gecnt++]);
          }

          if (f2 && ty->var_size > 8) {
            gen_pop("rax");
            println("  movq %%rax, %%xmm%d", flcnt++);
          } else if (ty->var_size > 8) {
            gen_pop("rax");
            println("  mov %%rax, %s", argregs64[gecnt++]);
          }
          break;
        }
        default:
          if (gecnt < 6) {
            gen_pop("rax");
            println("  mov %%rax, %s", argregs64[gecnt++]);
          } else {
            stcnt++;
          }
      }
    }

    println("  mov $%s, %%r10", node->func->name);
    println("  call *%%r10");
    gen_emptypop(stcnt);

    if (is_struct_type(ty)) {
      bool ld1 = is_struct_pass_by_stack(ty->ret_ty, 0, 0, true);
      bool ld2 = is_struct_pass_by_stack(ty->ret_ty, 0, 0, false);

      println("  sub $16, %%rsp");
      if (ld1 != ld2) {
        println("  fstpt (%%rsp)");
        println("  mov %%rsp, %%rax");
      } else if (!ld1) {
        bool f1 = has_only_float(ty->ret_ty, 0, 8, 0);
        bool f2 = has_only_float(ty->ret_ty, 8, 16, 0);

        int flcnt = 0, gecnt = 0;

        if (f1) {
          println("  movq %%xmm0, (%%rsp)");
          flcnt++;
        } else {
          println("  movq %%rax, (%%rsp)");
          gecnt++;
        }

        if (ty->ret_ty->var_size > 8) {
          if (f2) {
            println("  movq %%xmm%d, 8(%%rsp)", flcnt);
          } else {
            println("  movq %s, 8(%%rsp)", gecnt == 0 ? "%rax" : "%rdx");
          }
        }
        println("  mov %%rsp, %%rax");
      }
      println("  add $16, %%rsp");
    }

    return;
  }

  if (node->lhs->ty->kind == TY_LDOUBLE) {
    compile_node(node->lhs);
    compile_node(node->rhs);

    switch (node->kind) {
      case ND_ADD:
        println("  faddp");
        return;
      case ND_SUB:
        println("  fsubrp");
        return;
      case ND_MUL:
        println("  fmulp");
        return;
      case ND_DIV:
        println("  fdivrp");
        return;
      case ND_EQ:
      case ND_NEQ:
      case ND_LC:
      case ND_LEC:
        println("  fcomip");
        println("  fstp %%st(0)");

        if (node->kind == ND_EQ) {
          println("  sete %%al");
        } else if (node->kind == ND_NEQ) {
          println("  setne %%al");
        } else if (node->kind == ND_LC) {
          println("  seta %%al");
        } else {
          println("  setae %%al");
        }

        println("  movzx %%al, %%rax");
        return;
      default:
        return;
    }
  }

  if (node->lhs->ty->kind == TY_FLOAT || node->lhs->ty->kind == TY_DOUBLE) {
    char *suffix = (node->lhs->ty->kind == TY_FLOAT) ? "s" : "d";

    compile_node(node->lhs);
    println("  movq %%xmm0, %%rax");
    gen_push("rax");

    compile_node(node->rhs);
    println("  movsd %%xmm0, %%xmm1");
    gen_pop("rax");
    println("  movd %%rax, %%xmm0");


    switch (node->kind) {
      case ND_ADD:
        println("  adds%s %%xmm1, %%xmm0", suffix);
        return;
      case ND_SUB:
        println("  subs%s %%xmm1, %%xmm0", suffix);
        return;
      case ND_MUL:
        println("  muls%s %%xmm1, %%xmm0", suffix);
        return;
      case ND_DIV:
        println("  divs%s %%xmm1, %%xmm0", suffix);
        return;
      case ND_EQ:
      case ND_NEQ:
      case ND_LC:
      case ND_LEC:
        println("  ucomis%s %%xmm0, %%xmm1", suffix);

        if (node->kind == ND_EQ) {
          println("  sete %%al");
          println("  setnp %%dl");
          println("  and %%dl, %%al");
        } else if (node->kind == ND_NEQ) {
          println("  setne %%al");
          println("  setp %%dl");
          println("  or %%dl, %%al");
        } else if (node->kind == ND_LC) {
          println("  seta %%al");
        } else {
          println("  setnb %%al");
        }

        println("  movzx %%al, %%rax");
        return;
      default:
        return;
    }
  }

  // lhs: rax, rhs: rdi
  compile_node(node->lhs);
  gen_push("rax");

  compile_node(node->rhs);
  println("  mov %rax, %%rdi");
  gen_pop("rax");

  // Default register is 32bit
  char *rax = "%eax", *rdi = "%edi", *rdx = "%edx";

  if (node->ty->kind == TY_LONG || node->ty->base != NULL || is_struct_type(node->ty)) {
    rax = "%rax";
    rdi = "%rdi";
    rdx = "%rdx";
  }

  // calculation
  switch (node->kind) {
    case ND_ADD:
      println("  add %s, %s", rdi, rax);
      break;
    case ND_SUB:
      println("  sub %s, %s", rdi, rax);
      break;
    case ND_MUL:
      println("  imul %s, %s", rdi, rax);
      break;
    case ND_DIV:
    case ND_REMAINDER:
      if (node->ty->is_unsigned) {
        println("  mov $0. %%rdx");
        println("  div %s", rdi);
      } else {
        if (node->lhs->ty->var_size == 8) {
          println("  cqo");
        } else {
          println("  cdq");
        }
        println("  idiv %s", rdi);
      }

      if (node->kind == ND_REMAINDER) {
        println("  mov %%rdx, %%rax");
      }
      break;
    case ND_LEFTSHIFT:
      println("  mov %%rdi, %%rcx");
      println("  sal %%cl, %s", rax);
      break;
    case ND_RIGHTSHIFT:
      println("  mov %%rdi, %%rcx");
      if (node->lhs->ty->is_unsigned) {
        println("  shr %%cl, %s", rax);
      } else {
        println("  sar %%cl, %s", rax);
      }
      break;
    case ND_EQ:
    case ND_NEQ:
    case ND_LC:
    case ND_LEC:
      println("  cmp %s, %s", rdi, rax);

      if (node->kind == ND_EQ) {
        println("  sete %%al");
      } else if (node->kind == ND_NEQ) {
        println("  setne %%al");
      } else if (node->kind == ND_LC) {
        if (node->lhs->ty->is_unsigned) {
          println("  setb %%al");
        } else {
          println("  setl %%al");
        }
      } else {
        if (node->lhs->ty->is_unsigned) {
          println("  setbe %%al");
        } else {
          println("  setle %%al");
        }
      }

      println("  movzx %%al, %%rax");
      break;
    case ND_BITWISEAND:
      println("  and %s, %s", rdi, rax);
      break;
    case ND_BITWISEXOR:
      println("  xor %s, %s", rdi, rax);
      break;
    case ND_BITWISEOR:
      println("  or %s, %s", rdi, rax);
      break;
    case ND_LOGICALAND:
      println("  cmp $0, %s", rax);
      println("  je 1f");
      println("  cmp $0, %s", rdi);
      println("  je 1f");
      println("  mov $1, %%rax");
      println("  jmp 2f");
      println("1:");
      println("  mov $0, %%rax");
      println("2:");
      break;
    case ND_LOGICALOR:
      println("  cmp $0, %s", rax);
      println("  jne 1f");
      println("  cmp $0, %s", rdi);
      println("  je 2f");
      println("1:");
      println("  mov $1, %%rax");
      println("  jmp 3f");
      println("2:");
      println("  mov $0, %%rax");
      println("3:");
      break;
    default:
      break;
  }
}

void codegen(Node *head, char *filename) {
  output_file = fopen(filename, "w");

  for (Node *node = head; node != NULL; node = node->next) {
    if (node->kind == ND_INIT || node->kind == ND_VAR) {
      gen_gvar_init(node);
      continue;
    }

    if (node->kind != ND_FUNC) {
      continue;
    }

    Obj *func = node->func;
    if (func->ty->is_prototype) {
      continue;
    }

    println(".globl %s", func->name);
    println(".text");
    println(".type %s, @function", func->name);
    println("%s:", func->name);

    // Prologue
    gen_push("rbp");
    println("  mov %%rsp, %%rbp");
    println("  sub $%d, %%rsp", func->vars_size);
    println("  movq $%d, -8(%%rbp)", func->vars_size);

    // Set arguments
    int flcnt = 0, gecnt = 0, stframe = 16;

    // Set return
    Type *ret_ty = func->ty->ret_ty;
    if (is_struct_type(ret_ty) && is_struct_pass_by_stack(ret_ty, flcnt, gecnt, true)) {
      gen_push("rdi");
      gecnt++;
    }
    
    gen_push("rdi");
    for (Obj *param = node->func->params; param != NULL; param = param->next) {
      gen_addr(new_var(NULL, param));
      gen_push("rax");

      switch (param->ty->kind) {
        case TY_CHAR:
        case TY_SHORT:
        case TY_INT:
        case TY_LONG:
        case TY_PTR:
          if (gecnt == 0) {
            println("  mov 8(%%rsp), %%rax");
            gecnt++;
          } else if (gecnt < 6) {
            println("  mov %s, %%rax", argregs64[gecnt++]);
          } else {
            println("  mov %d(%%rbp), %%rax", stframe);
            stframe += 8;
          }
          gen_store(param->ty);
          break;
        case TY_FLOAT:
          if (flcnt < 8) {
            println("  movss %%xmm%d, %%xmm0", flcnt++);
          } else {
            println("  movd %d(%%rbp), %%xmm0", stframe);
            stframe += 8;
          }
          gen_store(param->ty);
          break;
        case TY_DOUBLE: 
          if (flcnt < 8) {
            println("  movsd %%xmm%d, %%xmm0", flcnt++);
          } else {
            println("  movq %d(%%rbp), %%xmm0", stframe);
            stframe += 8;
          }
          gen_store(param->ty);
          break;
        case TY_LDOUBLE:
          println("  fldt %d(%%rbp)", stframe);
          stframe += 16;
          gen_store(param->ty);
          break;
        case TY_STRUCT:
        case TY_UNION:
          if (!is_struct_pass_by_stack(param->ty, flcnt, gecnt, false)) {
            bool f1 = has_only_float(param->ty, 0, 8, 0);
            bool f2 = has_only_float(param->ty, 8, 16, 0);

            for (int i = 0; i < 2; i++) {
              if (param->ty->var_size <= 8 * i) {
                break;
              }

              int mvsize = param->ty->var_size % 8;  // QWORD is 0 or 8
              if (i == 0 && param->ty->var_size >= 8) {
                mvsize = 8;
              }

              println("  mov (%%rsp), %%rax");
              println("  add $%d, %%rax", 8 * i);
              if (i == 0 ? f1 : f2) {
                switch (mvsize) {
                  case 4:
                    println("  movd %%xmm%d, (%%rax)", flcnt++);
                    break;
                  default:
                    println("  movq %%xmm%d, (%%rax)", flcnt++);
                }
              } else {
                println("  mov 8(%%rsp), %%rdi");
                switch (mvsize) {
                  case 1:
                    println("  mov %s, (%%rax)", argregs8[gecnt++]);
                    break;
                  case 2:
                    println("  mov %s, (%%rax)", argregs16[gecnt++]);
                    break;
                  case 4:
                    println("  mov %s, (%%rax)", argregs32[gecnt++]);
                    break;
                  default:
                    println("  mov %s, (%%rax)", argregs64[gecnt++]);
                }
              }
            }
            gen_pop("rax");
          } else {
            gen_push("rsi");
            gen_push("rcx");
            println("  mov 16(%%rsp), %%rdi");
            println("  lea %d(%%rbp), %%rsi", stframe);
            println("  mov $%d, %%rcx", param->ty->var_size);
            println("  rep movsb");
            gen_pop("rcx");
            gen_pop("rsi");
            gen_pop("rax");
            stframe += align_to(param->ty->var_size, 8);
          }
        default:
          continue;
      }
    }
    gen_pop("rdi");

    compile_node(node->deep);

    println("  mov %%rbp, %%rsp");
    gen_pop("rbp");
    println("  ret");
  }
  fclose(output_file);
}
