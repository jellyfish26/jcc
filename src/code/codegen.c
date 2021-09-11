#include "code/codegen.h"
#include "parser/parser.h"
#include "token/tokenize.h"

#include <stdarg.h>
#include <stdbool.h>
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
  println("  push %s", reg);
}

static void gen_pop(const char *reg) {
  println("  pop %s", reg);
}

static void gen_emptypop(int num) {
  println("  add rsp, %d", num * 8);
}

// Compute the address of a given node.
// In the case of a local variable, it computes the relative address to the base pointer,
// and stores the absolute address in the RAX register.
static void gen_addr(Node *node) {
  switch (node->kind) {
    case ND_VAR:
      if (node->var->is_global) {
        println("  mov rax, offset %s", node->var->name);
        return;
      }

      println("  mov rax, rbp");
      println("  sub rax, %d", node->var->offset);
      return;
    case ND_CONTENT:
      compile_node(node->lhs);
      return;
    default:
      errorf_tkn(ER_COMPILE, node->tkn, "Not a variable.");
  }
}

static void gen_load(Type *ty) {
  if (ty->kind == TY_ARRAY) {
    // If it is an array or a string, it will automatically be treated as a pointer
    // and we cannot load the content direclty.
    return;
  }

  if (ty->kind == TY_FLOAT) {
    println("  movss xmm0, DWORD PTR [rax]");
    return;
  } else if (ty->kind == TY_DOUBLE) {
    println("  movsd xmm0, QWORD PTR [rax]");
    return;
  } else if (ty->kind == TY_LDOUBLE) {
    println("  fld TBYTE PTR [rax]");
    return;
  }

  char *unsi = ty->is_unsigned ? "movz" : "movs";

  // When char and short size values are loaded with mov instructions,
  // they may contain garbage in the lower 32bits,
  // so we are always extended to int.
  if (ty->var_size == 1) {
    println("  %sx eax, BYTE PTR [rax]", unsi);
  } else if (ty->var_size == 2) {
    println("  %sx eax, WORD PTR [rax]", unsi);
  } else if (ty->var_size == 4) {
    println("  mov eax, DWORD PTR [rax]");
  } else {
    println("  mov rax, QWORD PTR [rax]");
  }
}

// Store the value of the rax register at the address pointed to by the top of the stack.
static void gen_store(Type *ty) {
  gen_pop("rdi");

  if (ty->kind == TY_FLOAT) {
    println("  movss DWORD PTR [rdi], xmm0");
    return;
  } else if (ty->kind == TY_DOUBLE) {
    println("  movsd QWORD PTR [rdi], xmm0");
    return;
  } else if (ty->kind == TY_LDOUBLE) {
    println("  fstp TBYTE PTR [rdi]");
    return;
  }

  if (ty->var_size == 1) {
    println("  mov BYTE PTR [rdi], al");
  } else if (ty->var_size == 2) {
    println("  mov WORD PTR [rdi], ax");
  } else if (ty->var_size == 4) {
    println("  mov DWORD PTR [rdi], eax");
  } else {
    println("  mov QWORD PTR [rdi], rax");
  }
}

static char *argregs8[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
static char *argregs16[] = {"di", "si", "dx", "cx", "r8w", "r9w"};
static char *argregs32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
static char *argregs64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static char i32i8[]  = "movsx eax, al";
static char i32u8[]  = "movzx eax, al";
static char i32i16[] = "movsx eax, ax";
static char i32u16[] = "movzx eax, ax";
static char i64i32[] = "movsxd rax, eax";
static char i64u32[] = "mov eax, eax";

static char i8f32[]  = "cvttss2si eax, xmm0\n  movsx eax, al";
static char u8f32[]  = "cvttss2si eax, xmm0\n  movzx eax, al";
static char i16f32[] = "cvttss2si eax, xmm0\n  movsx eax, ax";
static char u16f32[] = "cvttss2si eax, xmm0\n  movzx eax, ax";
static char i32f32[] = "cvttss2si eax, xmm0";
static char u32f32[] = "cvttss2si rax, xmm0\n  mov eax, eax";
static char i64f32[] = "cvttss2si rax, xmm0";
static char u64f32[] = 
    "sub rsp, 8\n"
  "  movss DWORD PTR [rsp+4], xmm0\n"
  "  mov DWORD PTR [rsp], 1593835520\n"
  "  comiss xmm0, DWORD PTR [rsp]\n"
  "  jnb 1f\n"
  "  movss xmm0, DWORD PTR [rsp+4]\n"
  "  cvttss2si rax, xmm0\n"
  "  jmp 2f\n"
  "1:\n"
  "  movss xmm0, DWORD PTR [rsp+4]\n"
  "  movss xmm1, DWORD PTR [rsp]\n"
  "  subss xmm0, xmm1\n"
  "  cvttss2si rax, xmm0\n"
  "  movabs rdx, -9223372036854775808\n"
  "  xor rax, rdx\n"
  "2:\n"
  "  add rsp, 8";

static char f32i8[]  = "movsx eax, al\n  cvtsi2ss xmm0, eax";
static char f32u8[]  = "movzx eax, al\n  cvtsi2ss xmm0, eax";
static char f32i16[] = "movsx eax, ax\n  cvtsi2ss xmm0, eax";
static char f32u16[] = "movzx eax, ax\n  cvtsi2ss xmm0, eax";
static char f32i32[] = "cvtsi2ss xmm0, eax";
static char f32u32[] = "mov eax, eax\n  cvtsi2ss xmm0, rax";
static char f32i64[] = "cvtsi2ss xmm0, rax";
static char f32u64[] = 
    "test rax, rax\n"
  "  pxor xmm0, xmm0\n"
  "  js 1f\n"
  "  cvtsi2ss xmm0, rax\n"
  "  jmp 2f\n"
  "1:\n"
  "  mov rdx, rax\n"
  "  shr rdx\n"
  "  and eax, 1\n"
  "  or rdx, rax\n"
  "  cvtsi2ss xmm0, rdx\n"
  "  addss xmm0, xmm0\n"
  "2:";

static char i8f64[]  = "cvttsd2si eax, xmm0\n  movsx eax, al";
static char u8f64[]  = "cvttsd2si eax, xmm0\n  movzx eax, al";
static char i16f64[] = "cvttsd2si eax, xmm0\n  movsx eax, ax";
static char u16f64[] = "cvttsd2si eax, xmm0\n  movzx eax, ax";
static char i32f64[] = "cvttsd2si eax, xmm0";
static char u32f64[] = "cvttsd2si rax, xmm0\n  mov eax, eax";
static char i64f64[] = "cvttsd2si rax, xmm0";
static char u64f64[] = 
    "sub rsp, 16\n"
  "  movsd QWORD PTR [rsp+8], xmm0\n"
  "  mov DWORD PTR [rsp], 0\n"
  "  mov DWORD PTR [rsp+4], 1138753536\n"
  "  comisd xmm0, QWORD PTR [rsp]\n"
  "  jnb 1f\n"
  "  movsd xmm0, QWORD PTR [rsp+8]\n"
  "  cvttsd2si rax, xmm0\n"
  "  jmp 2f\n"
  "1:\n"
  "  movsd xmm0, QWORD PTR [rsp+8]\n"
  "  movsd xmm1, QWORD PTR [rsp]\n"
  "  subsd xmm0, xmm1\n"
  "  cvttsd2si rax, xmm0\n"
  "  movabs rdx, -9223372036854775808\n"
  "  xor rax, rdx\n"
  "2:\n"
  "  add rsp, 16";

static char f32f64[] = "cvtsd2ss xmm0, xmm0";

static char f64i8[]  = "movsx eax, al\n  cvtsi2sd xmm0, eax";
static char f64u8[]  = "movzx eax, al\n  cvtsi2sd xmm0, eax";
static char f64i16[] = "movsx eax, ax\n  cvtsi2sd xmm0, eax";
static char f64u16[] = "movzx eax, ax\n  cvtsi2sd xmm0, eax";
static char f64i32[] = "cvtsi2sd xmm0, eax";
static char f64u32[] = "mov eax, eax\n  cvtsi2sd xmm0, rax";
static char f64i64[] = "cvtsi2sd xmm0, rax";
static char f64u64[] = 
    "test rax, rax\n"
  "  pxor xmm0, xmm0\n"
  "  js 1f\n"
  "  cvtsi2sd xmm0, rax\n"
  "  jmp 2f\n"
  "1:\n"
  "  mov rdx, rax\n"
  "  shr rdx\n"
  "  and eax, 1\n"
  "  or rdx, rax\n"
  "  cvtsi2sd xmm0, rdx\n"
  "  addsd xmm0, xmm0\n"
  "2:";
static char f64f32[] = "cvtss2sd xmm0, xmm0";

#define F80_TO_INT_1 \
    "sub rsp, 16\n" \
  "  fnstcw WORD PTR [rsp+12]\n" \
  "  movzx eax, WORD PTR [rsp+12]\n" \
  "  or ah, 12\n" \
  "  mov WORD PTR [rsp+8], ax\n" \
  "  fldcw WORD PTR [rsp+8]\n"

#define F80_TO_INT_2 \
  "  fldcw WORD PTR [rsp+12]\n"

#define F80_TO_INT_3 \
  "  add rsp, 16"

static char i8f80[]  = F80_TO_INT_1 "  fistp WORD PTR [rsp]\n" F80_TO_INT_2 "  mov al, BYTE PTR [rsp]\n  movsx eax, al\n" F80_TO_INT_3;
static char u8f80[]  = F80_TO_INT_1 "  fistp WORD PTR [rsp]\n" F80_TO_INT_2 "  movzx eax, BYTE PTR [rsp]\n" F80_TO_INT_3;
static char i16f80[] = F80_TO_INT_1 "  fistp WORD PTR [rsp]\n" F80_TO_INT_2 "  xor rax, rax\n  mov ax, WORD PTR [rsp]\n" F80_TO_INT_3;
static char u16f80[] = F80_TO_INT_1 "  fistp DWORD PTR [rsp]\n" F80_TO_INT_2 "  movzx eax, WORD PTR [rsp]\n" F80_TO_INT_3;
static char i32f80[] = F80_TO_INT_1 "  fistp DWORD PTR [rsp]\n" F80_TO_INT_2 "  mov eax, DWORD PTR [rsp]\n" F80_TO_INT_3;
static char u32f80[] = F80_TO_INT_1 "  fistp QWORD PTR [rsp]\n" F80_TO_INT_2 "  movzx rax, DWORD PTR [rsp]\n" F80_TO_INT_3;
static char i64f80[] = F80_TO_INT_1 "  fistp QWORD PTR [rsp]\n" F80_TO_INT_2 "  mov rax, QWORD PTR [rsp]\n" F80_TO_INT_3;

static char u64f80[] =
    "sub rsp, 32\n"
  "  fstp TBYTE PTR [rsp+16]\n"
  "  mov DWORD PTR [rsp], 0\n"
  "  mov DWORD PTR [rsp+4], -2147483648\n"
  "  mov DWORD PTR [rsp+8], 16446\n"
  "  mov DWORD PTR [rsp+12], 0\n"
  "  fld TBYTE PTR [rsp]\n"
  "  fld TBYTE PTR [rsp+16]\n"
  "  fcomip st, st(1)\n"
  "  fstp st(0)\n"
  "  jnb 1f\n"
  "  fld TBYTE PTR [rsp+16]\n"
  "  " F80_TO_INT_1
  "  fistp QWORD PTR [rsp]\n"
  F80_TO_INT_2
  "  mov rax, QWORD PTR [rsp]\n"
  F80_TO_INT_3 "\n"
  "  jmp 2f\n"
  "1:"
  "  fld TBYTE PTR [rsp+16]\n"
  "  fld TBYTE PTR [rsp]\n"
  "  fsubp st(1), st\n"
  "  " F80_TO_INT_1
  "  fistp QWORD PTR [rsp]\n"
  F80_TO_INT_2
  "  mov rax, QWORD PTR [rsp]\n"
  F80_TO_INT_3 "\n"
  "  movabs rdi, -9223372036854775808\n"
  "  xor rax, rdi\n"
  "2:\n"
  "  add rsp, 32";

static char f32f80[] = 
    "sub rsp, 4\n"
  "  fstp DWORD PTR [rsp+4]\n"
  "  movss xmm0, DWORD PTR [rsp+4]\n"
  "  add rsp, 8\n";

static char f64f80[] = 
    "sub rsp, 8\n"
  "  fstp QWORD PTR [rsp+8]\n"
  "  movsd xmm0, QWORD PTR [rsp+8]\n"
  "  add rsp, 8\n";

static char f80i8[]  = "sub rsp, 2\n  movsx ax, al\n  mov WORD PTR [rsp], ax\n  fild WORD PTR [rsp]\n  add rsp, 2";
static char f80u8[]  = "sub rsp, 2\n  movzx ax, al\n  mov WORD PTR [rsp], ax\n  fild WORD PTR [rsp]\n  add rsp, 2";
static char f80i16[] = "sub rsp, 2\n  mov WORD PTR [rsp], ax\n  fild WORD PTR [rsp]\n  add rsp, 2";
static char f80u16[] = "sub rsp, 4\n  movzx DWORD PTR [rsp], ax\n  fild DWORD PTR [rsp]\n  add rsp, 4";
static char f80i32[] = "sub rsp, 4\n  mov DWORD PTR [rsp], eax\n  fild DWORD PTR [rsp]\n  add rsp, 4";
static char f80u32[] = "sub rsp, 8\n  movzx QWORD PTR [rsp], eax\n  fild QWORD PTR [rsp]\n  add rsp, 8";
static char f80i64[] = "sub rsp, 8\n  mov QWORD PTR [rsp], rax\n  fild QWORD PTR [rsp]\n  add rsp, 8";

static char f80f32[] = "sub rsp, 4\n  movss DWORD PTR [rsp], xmm0\n  fld DWORD PTR [rsp]\n  add rsp, 4";
static char f80f64[] = "sub rsp, 8\n  movsd QWORD PTR [rsp], xmm0\n  fld QWORD PTR [rsp]\n  add rsp, 8";

static char f80u64[] =
    "sub rsp, 32\n"
  "  mov DWORD PTR [rsp], 0\n"
  "  mov DWORD PTR [rsp+4], -2147483648\n"
  "  mov DWORD PTR [rsp+8], 16447\n"
  "  mov DWORD PTR [rsp+12], 0\n"
  "  mov QWORD PTR [rsp+16], rax\n"
  "  fild QWORD PTR [rsp+16]\n"
  "  cmp QWORD PTR [rsp+16], 0\n"
  "  jns 1f\n"
  "  fld TBYTE PTR [rsp]\n"
  "  faddp st(1), st\n"
  "1:\n"
  "  add rsp, 32";

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
  ty = extract_ty(ty);

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
  println("  cmp rax, 0");
  println("  je .Lfalse%d", label);

  compile_node(node->lhs);
  println("  jmp .Lnext%d", label);
  println(".Lfalse%d:", label);

  compile_node(node->rhs);
  println(".Lnext%d:", label);
}

static void gen_lvar_init(Node *node) {
  gen_addr(node->lhs);
  int offset = 0;

  for (Node *expr = node->rhs; expr != NULL; expr = expr->lhs) {
    gen_push("rax");
    if (expr->init != NULL) {
      switch (expr->ty->kind) {
        case TY_STRUCT:
        case TY_UNION:
          if (expr->init->kind == ND_ASSIGN) {
            compile_node(expr->init);
            gen_addr(expr->init->lhs);
          } else {
            gen_addr(expr->init);
          }
          println("  mov rsi, rax");
          gen_pop("rdi");
          println("  mov rcx, %d", expr->ty->var_size);
          println("  rep movsb");
          println("  mov rax, rdi");
          break;
        default:
          compile_node(expr->init);
          gen_store(expr->ty);
          println("  mov rax, rdi");
          println("  add rax, %d", expr->ty->var_size);
      }
    } else {
      gen_pop("rdi");
      println("  xor rax, rax");
      println("  mov rcx, %d", expr->ty->var_size);
      println("  rep stosb");
      println("  mov rax, rdi");
    }
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


  for (Node *init = node->rhs; init != NULL; init = init->lhs) {
    if (init->init == NULL) {
      println("  .zero %d", init->ty->var_size);
      continue;
    }

    if (init->ty->kind == TY_FLOAT) {
      float *ptr = calloc(1, sizeof(float));
      *ptr = (float)init->init->fval;
      println("  .long %d", *(int*)ptr);
      free(ptr);
      continue;
    }

    if (init->ty->kind == TY_DOUBLE) {
      double *ptr = calloc(1, sizeof(double));
      *ptr = (double)init->init->fval;
      println("  .long %d", *(int*)ptr);
      println("  .long %d", *((int*)ptr + 1));
      free(ptr);
      continue;
    }

    if (init->ty->kind == TY_LDOUBLE) {
      long double *ptr = calloc(1, sizeof(double));
      *ptr = init->init->fval;
      for (int i = 0; i < 4; i++) {
        println("  .long %d", *((int*)ptr + i));
      }
      free(ptr);
      continue;
    }

    char *asm_ty;
    switch (init->ty->var_size) {
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

    char *ptr_label = init->init->var->name;
    if (ptr_label == NULL) {
      println("  %s %d", asm_ty, init->init->val);
    } else {
      println("  %s %s%+d", asm_ty, ptr_label, init->init->val);
    }
  }
}

static bool has_only_float(Type *ty, int begin, int end, int offset) {
  if (ty->kind == TY_STRUCT) {
    for (Member *member = ty->member; member != NULL; member = member->next) {
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

static void push_argsre(Node *node, bool pass_stack) {
  if (node == NULL) {
    return;
  }
  push_argsre(node->next, pass_stack);

  if ((pass_stack && !node->pass_by_stack) || (!pass_stack && node->pass_by_stack)) {
    return;
  }

  Type *ty = extract_ty(node->lhs->ty);
  switch (ty->kind) {
    case TY_FLOAT:
    case TY_DOUBLE:
      compile_node(node->lhs);
      println("  movd rax, xmm0");
      gen_push("rax");
      break;
    case TY_LDOUBLE:
      compile_node(node->lhs);
      println("  sub rsp, 16");
      println("  fstp TBYTE PTR [rsp]");
      break;
    case TY_STRUCT: {
      println("  sub rsp, %d", align_to(ty->var_size, 8));
      gen_addr(node->lhs);
      println("  mov rsi, rax");
      println("  mov rdi, rsp");
      println("  mov rcx, %d", ty->var_size);
      println("  rep movsb");
      break;
    }
    default:
      compile_node(node->lhs);
      gen_push("rax");
  }
}

static void push_args(Node *node, bool pass_stack) {
  // floating cnt, general cnt
  // pass by stack if flcnt >= 8 or gecnt >= 6
  int flcnt = 0, gecnt = 0;

  for (Node *arg = node->args; arg != NULL; arg = arg->next) {
    Type *arg_ty = extract_ty(arg->lhs->ty);

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
      case TY_STRUCT: {
        if (arg_ty->var_size > 16 || arg_ty->member->ty->kind == TY_LDOUBLE) {
          arg->pass_by_stack = true;
          break;
        }

        bool f1 = has_only_float(arg_ty, 0, 8, 0);
        bool f2 = has_only_float(arg_ty, 8, 16, 0);

        if (flcnt + f1 + f2 < 8 && gecnt + !f1 + !f2 < 6) {
          flcnt += f1 + f2;
          gecnt += !f1 + !f2;
        } else {
          arg->pass_by_stack = true;
          break;
        }
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
    switch (node->ty->kind) {
      case TY_CHAR:
      case TY_SHORT:
      case TY_INT:
      case TY_LONG:
        println("  mov rax, %ld", node->val);
        break;
      case TY_FLOAT: {
        float *ptr = calloc(1, sizeof(float));
        *ptr = (float)node->fval;
        println("  sub rsp, 4");
        println("  mov DWORD PTR [rsp], %d", *(int*)ptr);
        println("  movss xmm0, DWORD PTR [rsp]");
        println("  add rsp, 4");
        break;
      }
      case TY_DOUBLE: {
        double *ptr = calloc(1, sizeof(double));
        *ptr = (double)node->fval;
        println("  sub rsp, 8");
        println("  mov DWORD PTR [rsp], %d", *(int*)ptr);
        println("  mov DWORD PTR [rsp+4], %d", *((int*)ptr + 1));
        println("  movsd xmm0, QWORD PTR [rsp]");
        println("  add rsp, 8");
        break;
      }
      case TY_LDOUBLE: {
        long double *ptr = calloc(1, sizeof(long double));
        *ptr = (long double)node->fval;
        println("  sub rsp, 16");
        println("  mov DWORD PTR [rsp], %d", *(int*)ptr);
        println("  mov DWORD PTR [rsp+4], %d", *((int*)ptr + 1));
        println("  mov DWORD PTR [rsp+8], %d", *((int*)ptr + 2));
        println("  mov DWORD PTR [rsp+12], %d", *((int*)ptr + 3));
        println("  fld TBYTE PTR [rsp]");
        println("  add rsp, 16");
      }
    }
    return;
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
        println("  mov rax, %ld", node->var->val);
      } else {
        gen_addr(node);
        gen_load(node->ty);
      }
      return;
    case ND_ADDR:
      gen_addr(node->lhs);
      return;
    case ND_ASSIGN: {
      if (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION) {
        gen_addr(node->lhs);
        gen_push("rax");

        if (node->rhs->kind == ND_ASSIGN) {
          gen_addr(node->rhs->lhs);
        } else {
          gen_addr(node->rhs);
        }

        println("  mov rsi, rax");
        gen_pop("  rdi");
        println("  mov rcx, %d", node->ty->var_size);
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
      compile_node(node->lhs);
      println("  mov rsp, rbp");
      gen_pop("rbp");
      println("  ret");
      return;
    case ND_IF: {
      int now_label = branch_label++;
      compile_node(node->cond);
      println("  cmp rax, 0");
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
        println("  cmp rax, 0");
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
      println("  cmp rax, 0");
      println("  jne %s", node->conti_label);

      println("%s:", node->break_label);
      return;
    }
    case ND_SWITCH: {
      int now_label = branch_label++;
      compile_node(node->cond);

      int cnt = 0;
      for (Node *expr = node->case_stmt; expr != NULL; expr = expr->case_stmt) {
        println("  cmp rax, %ld", expr->val);
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
      println("  not rax");
      return;
    default:
      break;
  }

  if (node->kind == ND_FUNCCALL) {
    Type *ty = node->func->ty;

    push_args(node, true);
    push_args(node, false);

    // floating cnt, general cnt, stack cnt
    // pass by stack if flcnt >= 9 or gecnt >= 6
    int flcnt = 0, gecnt = 0, stcnt = 0;

    for (Node *arg = node->args; arg != NULL; arg = arg->next) {
      Type *ty = extract_ty(arg->lhs->ty);

      switch (ty->kind) {
        case TY_FLOAT:
        case TY_DOUBLE:
          if (flcnt < 8) {
            gen_pop("rax");
            println("  movd xmm%d, rax", flcnt);
            flcnt++;
          } else {
            stcnt++;
          }
          break;
        case TY_LDOUBLE:
          stcnt += 2;
          break;
        case TY_STRUCT: {
          if (arg->pass_by_stack) {
            stcnt += align_to(ty->var_size, 8) / 8;
            break;
          }

          bool f1 = has_only_float(ty, 0, 8, 0);
          bool f2 = has_only_float(ty, 8, 16, 0);

          gen_pop("rax");
          if (f1) {
            println("  movq xmm%d, rax", flcnt++);
          } else {
            println("  mov %s, rax", argregs64[gecnt++]);
          }

          if (f2 && ty->var_size > 8) {
            gen_pop("rax");
            println("  movq xmm%d, rax", flcnt++);
          } else if (ty->var_size > 8) {
            gen_pop("rax");
            println("  mov %s, rax", argregs64[gecnt++]);
          }
          break;
        }
        default:
          if (gecnt < 6) {
            gen_pop("rax");
            println("  mov %s, rax", argregs64[gecnt]);
            gecnt++;
          } else {
            stcnt++;
          }
      }
    }

    println("  call %s", node->func->name);
    gen_emptypop(stcnt);
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
        println("  fsubp");
        return;
      case ND_MUL:
        println("  fmulp");
        return;
      case ND_DIV:
        println("  fdivp");
        return;
      case ND_EQ:
      case ND_NEQ:
      case ND_LC:
      case ND_LEC:
        println("  fcomip");
        println("  fstp st(0)");

        if (node->kind == ND_EQ) {
          println("  sete al");
        } else if (node->kind == ND_NEQ) {
          println("  setne al");
        } else if (node->kind == ND_LC) {
          println("  seta al");
        } else {
          println("  setae al");
        }

        println("  movzx rax, al");
        return;
    }
  }

  if (node->lhs->ty->kind == TY_FLOAT || node->lhs->ty->kind == TY_DOUBLE) {
    char *suffix = (node->lhs->ty->kind == TY_FLOAT) ? "s" : "d";

    compile_node(node->lhs);
    println(" movd rax, xmm0");
    gen_push("rax");

    compile_node(node->rhs);
    println("  movap%s xmm1, xmm0", suffix);
    gen_pop("rax");
    println("  movd xmm0, rax");


    switch (node->kind) {
      case ND_ADD:
        println("  adds%s xmm0, xmm1", suffix);
        return;
      case ND_SUB:
        println("  subs%s xmm0, xmm1", suffix);
        return;
      case ND_MUL:
        println("  muls%s xmm0, xmm1", suffix);
        return;
      case ND_DIV:
        println("  divs%s xmm0, xmm1", suffix);
        return;
      case ND_EQ:
      case ND_NEQ:
      case ND_LC:
      case ND_LEC:
        println("  ucomis%s xmm1, xmm0", suffix);

        if (node->kind == ND_EQ) {
          println("  sete al");
          println("  setnp dl");
          println("  and al, dl");
        } else if (node->kind == ND_NEQ) {
          println("  setne al");
          println("  setp dl");
          println("  or al, dl");
        } else if (node->kind == ND_LC) {
          println("  seta al");
        } else {
          println("  setnb al");
        }

        println("  movzx rax, al");
        return;
    }
  }

  // lhs: rax, rhs: rdi
  compile_node(node->lhs);
  gen_push("rax");

  compile_node(node->rhs);
  println("  mov rdi, rax");
  gen_pop("rax");

  // Default register is 32bit
  char *rax = "eax", *rdi = "edi", *rdx = "edx";

  if (node->lhs->ty->kind == TY_LONG || node->lhs->ty->base != NULL) {
    rax = "rax";
    rdi = "rdi";
    rdx = "rdx";
  }

  // calculation
  switch (node->kind) {
    case ND_ADD:
      println("  add %s, %s", rax, rdi);
      break;
    case ND_SUB:
      println("  sub %s, %s", rax, rdi);
      break;
    case ND_MUL:
      println("  imul %s, %s", rax, rdi);
      break;
    case ND_DIV:
    case ND_REMAINDER:
      if (node->ty->is_unsigned) {
        println("  mov rdx, 0");
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
        println("  mov rax, rdx");
      }
      break;
    case ND_LEFTSHIFT:
      println("  mov rcx, rdi");
      println("  sal %s, cl", rax);
      break;
    case ND_RIGHTSHIFT:
      println("  mov rcx, rdi");
      if (node->lhs->ty->is_unsigned) {
        println("  shr %s, cl", rax);
      } else {
        println("  sar %s, cl", rax);
      }
      break;
    case ND_EQ:
    case ND_NEQ:
    case ND_LC:
    case ND_LEC:
      println("  cmp %s, %s", rax, rdi);

      if (node->kind == ND_EQ) {
        println("  sete al");
      } else if (node->kind == ND_NEQ) {
        println("  setne al");
      } else if (node->kind == ND_LC) {
        if (node->lhs->ty->is_unsigned) {
          println("  setb al");
        } else {
          println("  setl al");
        }
      } else {
        if (node->lhs->ty->is_unsigned) {
          println("  setbe al");
        } else {
          println("  setle al");
        }
      }

      println("  movzx rax, al");
      break;
    case ND_BITWISEAND:
      println("  and %s, %s", rax, rdi);
      break;
    case ND_BITWISEXOR:
      println("  xor %s, %s", rax, rdi);
      break;
    case ND_BITWISEOR:
      println("  or %s, %s", rax, rdi);
      break;
    case ND_LOGICALAND:
      println("  cmp %s, 0", rax);
      println("  je 1f");
      println("  cmp %s, 0", rdi);
      println("  je 1f");
      println("  mov rax, 1");
      println("  jmp 2f");
      println("1:");
      println("  mov rax, 0");
      println("2:");
      break;
    case ND_LOGICALOR:
      println("  cmp %s, 0", rax);
      println("  jne 1f");
      println("  cmp %s, 0", rdi);
      println("  je 2f");
      println("1:");
      println("  mov rax, 1");
      println("  jmp 3f");
      println("2:");
      println("  mov rax, 0");
      println("3:");
      break;
    default:
      break;
  }
}

void codegen(Node *head, char *filename) {
  output_file = fopen(filename, "w");
  println(".intel_syntax noprefix");

  for (Node *node = head; node != NULL; node = node->next) {
    if (node->kind != ND_FUNC) {
      gen_gvar_init(node);
      continue;
    }

    Obj *func = node->func;
    if (func->ty->is_prototype) {
      continue;
    }

    println(".global %s", func->name);
    println(".text");
    println("%s:", func->name);

    // Prologue
    gen_push("rbp");
    println("  mov rbp, rsp");
    println("  sub rsp, %d", func->vars_size);

    // Set arguments
    int flcnt = 0, gecnt = 0, stframe = 16;
    
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
            println("  mov rax, QWORD PTR [rsp+8]");
            gecnt++;
          } else if (gecnt < 6) {
            println("  mov rax, %s", argregs64[gecnt++]);
          } else {
            println("  mov rax, QWORD PTR [rbp+%d]", stframe);
            stframe += 8;
          }
          gen_store(param->ty);
          break;
        case TY_FLOAT:
        case TY_DOUBLE: {
          char *suffix = (param->ty->kind == TY_FLOAT) ? "s" : "d";

          if (flcnt < 8) {
            println("  movap%s xmm0, xmm%d", suffix, flcnt++);
          } else {
            println("  movd xmm0, QWORD PTR [rbp+%d]", stframe);
            stframe += 8;
          }
          gen_store(param->ty);
          break;
        }
        case TY_LDOUBLE:
          println("  fld TBYTE PTR [rbp+%d]", stframe);
          stframe += 16;
          gen_store(param->ty);
          break;
        case TY_STRUCT: {
          int stsize = 0;

          if (param->ty->var_size > 16 || param->ty->member->ty->kind == TY_LDOUBLE) {
            stsize = param->ty->var_size;
          } else {
            bool f1 = has_only_float(param->ty, 0, 8, 0);
            bool f2 = has_only_float(param->ty, 8, 16, 0);

            if (flcnt + f1 + f2 >= 8 || gecnt + !f1 + !f2 >= 6) {
              stsize = param->ty->var_size;
            }
          }

          if (stsize == 0) {
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

              println("  mov rax, QWORD PTR [rsp]");
              println("  add rax, %d", 8 * i);
              if (i == 0 ? f1 : f2) {
                switch (mvsize) {
                  case 4:
                    println(" movd DWORD PTR [rax], xmm%d", flcnt++);
                    break;
                  default:
                    println(" movq QWORD PTR [rax], xmm%d", flcnt++);
                }
              } else {
                switch (mvsize) {
                  case 1:
                    println("  mov BYTE PTR [rax], %s", argregs8[gecnt++]);
                    break;
                  case 2:
                    println("  mov WORD PTR [rax], %s", argregs16[gecnt++]);
                    break;
                  case 4:
                    println("  mov DWORD PTR [rax], %s", argregs32[gecnt++]);
                    break;
                  default:
                    println("  mov QWORD PTR [rax], %s", argregs64[gecnt++]);
                }
              }
            }
            gen_pop("rax");
          } else {
            gen_push("rsi");
            gen_push("rcx");
            println("  mov rdi, QWORD PTR [rsp+16]");
            println("  lea rsi, [rbp+%d]", stframe);
            println("  mov rcx, %d", stsize);
            println("  rep movsb");
            gen_pop("rcx");
            gen_pop("rsi");
            gen_pop("rax");
            stframe += stsize;
          }
        }
      }
    }
    gen_pop("rdi");

    compile_node(node->deep);

    println("  mov rsp, rbp");
    gen_pop("rbp");
    println("  ret");
  }
  fclose(output_file);
}
