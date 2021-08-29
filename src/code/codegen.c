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
int get_type_size(Type *type);

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
      // String literal
      if (node->use_var->ty->kind == TY_STR) {
        println("  mov rax, offset .LC%d", node->use_var->offset);
        return;
      }

      if (node->use_var->is_global) {
        println("  mov rax, offset %s", node->use_var->name);
        return;
      }

      println("  mov rax, rbp");
      println("  sub rax, %d", node->use_var->offset);
      return;
    case ND_CONTENT:
      compile_node(node->lhs);
      return;
    default:
      errorf_tkn(ER_COMPILE, node->tkn, "Not a variable.");
  }
}

static void gen_load(Type *ty) {
  if (ty->kind == TY_ARRAY || ty->kind == TY_STR) {
    // If it is an array or a string, it will automatically be treated as a pointer
    // and we cannot load the content direclty.
    return;
  }

  if (ty->kind == TY_FLOAT) {
    println("  movss xmm0, QWORD PTR [rax]");
    return;
  } else if (ty->kind == TY_DOUBLE) {
    println("  movsd xmm0, QWORD PTR [rax]");
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

// Store the value of the xmm0 register at the address pointed to by the top of the stack.
static void gen_fstore(Type *ty) {
  gen_pop("rdi");

  if (ty->var_size == 4) {
    println("  movss DWORD PTR [rdi], xmm0");
  } else if (ty->var_size == 8) {
    println("  movsd QWORD PTR [rdi], xmm0");
  }
}

static const char *args_reg[] = {
  "rdi", "rsi", "rdx", "rcx", "r8", "r9",
};

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
    "mov rax, 9223372036854775807\n"
  "  cvtsi2ss xmm1, rax\n"
  "  xor rax, rax\n"
  "  xor rdi, rdi\n"
  "  comiss xmm0, xmm1\n"
  "  jb 1f\n"
  "  mov rdi, 9223372036854775808\n"
  "  subss xmm0, xmm1\n"
  "1:\n"
  "  cvttss2si rax, xmm0\n"
  "  add rax, rdi";

static char i8f64[]  = "cvttsd2si eax, xmm0\n  movsx eax, al";
static char u8f64[]  = "cvttsd2si eax, xmm0\n  movzx eax, al";
static char i16f64[] = "cvttsd2si eax, xmm0\n  movsx eax, ax";
static char u16f64[] = "cvttsd2si eax, xmm0\n  movzx eax, ax";
static char i32f64[] = "cvttsd2si eax, xmm0";
static char u32f64[] = "cvttsd2si rax, xmm0\n  mov eax, eax";
static char i64f64[] = "cvttsd2si rax, xmm0";
static char u64f64[] = 
    "mov rax, 9223372036854775807\n"
  "  cvtsi2sd xmm1, rax\n"
  "  xor rax, rax\n"
  "  xor rdi, rdi\n"
  "  comisd xmm0, xmm1\n"
  "  jb 1f\n"
  "  mov rdi, 9223372036854775808\n"
  "  subsd xmm0, xmm1\n"
  "1:\n"
  "  cvttsd2si rax, xmm0\n"
  "  add rax, rdi";

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

static char *cast_table[][10] = {
// i8     i16     i32     i64     u8     u16     u32     u64     f32   f64     to/from
  {NULL,  NULL,   NULL,   i64i32, i32u8, i32u16, NULL,   i64i32, NULL, f64i8},  // i8
  {i32i8, NULL,   NULL,   i64i32, i32u8, i32u16, NULL,   i64i32, NULL, f64i16}, // i16
  {i32i8, i32i16, NULL,   i64i32, i32u8, i32u16, NULL,   i64i32, NULL, f64i32}, // i32
  {i32i8, i32i16, NULL,   NULL,   i32u8, i32u16, NULL,   NULL,   NULL, f64i64}, // i64

  {i32i8, NULL,   NULL,   i64i32, i32u8, i32u16, NULL,   i64i32, NULL, f64u8},  // u8
  {i32i8, i32i16, NULL,   i64i32, i32u8, NULL,   NULL,   i64i32, NULL, f64u16}, // u16
  {i32i8, i32i16, NULL,   i64u32, i32u8, i32u16, NULL,   i64u32, NULL, f64u32}, // u32
  {i32i8, i32i16, NULL,   NULL,   i32u8, i32u16, NULL,   NULL,   NULL, f64u64}, // u64

  {i8f32, i16f32, i32f32, i64f32, u8f32, u16f32, u32f32, u64f32, NULL, f64f32}, // f32
  {i8f64, i16f64, i32f64, i64f64, u8f64, u16f64, u32f64, u64f64, NULL, NULL},   // f64
};

static int get_type_idx(Type *type) {
  int ret;
  switch (type->kind) {
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
    case TY_DOUBLE:
      ret = 9;
      break;
    default:
      return 0;
  };
  if (type->is_unsigned) {
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

static void gen_comp(char *comp_op, char *lreg, char *rreg) {
  println("  cmp %s, %s", lreg, rreg);
  println("  %s al", comp_op);
  println("  movzx rax, al");
}

int branch_label = 0;

// Right to left
void expand_assign(Node *node) {
  gen_addr(node->lhs);
  gen_push("rax");

  compile_node(node->rhs);

  switch (node->ty->kind) {
    case TY_FLOAT:
    case TY_DOUBLE:
    case TY_LDOUBLE:
      gen_fstore(node->ty);
      break;
    default:
      gen_store(node->ty);
  }
}

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

  for (Node *init = node->rhs; init != NULL; init = init->lhs) {
    gen_push("rax");
    if (init->init != NULL) {
      compile_node(init->init);
    } else {
      println("  mov rax, 0");
    }

    switch (init->ty->kind) {
      case TY_DOUBLE:
        gen_fstore(init->ty);
        break;
      default:
        gen_store(init->ty);
    }

    println("  mov rax, rdi");
    println("  add rax, %d", init->ty->var_size);
  }
}

static void gen_gvar_init(Node *node) {
  println(".data");
  println("%s:", node->lhs->use_var->name);


  for (Node *init = node->rhs; init != NULL; init = init->lhs) {
    char *asm_ty;
    switch (init->ty->kind) {
      case TY_CHAR:
        asm_ty = ".byte";
        break;
      case TY_SHORT:
        asm_ty = ".short";
        break;
      case TY_INT:
        asm_ty = ".long";
        break;
      case TY_LONG:
      case TY_PTR:
        asm_ty = ".quad";
        break;
      default:
        return;
    }

    if (init->init == NULL) {
      println("  %s 0", asm_ty);
      continue;
    }

    char *ptr_label = init->init->use_var->name;
    if (ptr_label == NULL) {
      println("  %s %d", asm_ty, init->init->val);
    } else {
      println("  %s %s%+d", asm_ty, ptr_label, init->init->val);
    }
  }
}

static void gen_gvar_define(Obj *var) {
  println(".data");

  if (var->ty->kind == TY_STR) {
    println(".LC%d:", var->offset);
    println("  .string \"%s\"", var->name);
    return;
  }

  if (var->ty->kind == TY_DOUBLE) {
    double *ptr = calloc(1, sizeof(double));
    *ptr = (double)var->fval;
    println(".LC%d:", var->offset);
    println("  .long %d", *(int*)ptr);
    println("  .long %d", *((int*)ptr + 1));
    free(ptr);
    return;
  }


  println("%s:", var->name);
  switch (var->ty->kind) {
    case TY_CHAR:
      println("  .zero 1");
      break;
    case TY_SHORT:
      println("  .zero 2");
      break;
    case TY_INT:
      println("  .zero 4");
      break;
    case TY_LONG:
    case TY_PTR:
    case TY_ARRAY:
      println("  .zero %d", var->ty->var_size);
      break;
    default:
      return;
  }
}

static void cnt_func_params(Node *node, int *general, int *floating) {
  Type *ty = node->func->ty;

  for (int i = 0; i < node->argc; i++) {
    TypeKind kind = (*(node->args + i))->lhs->ty->kind;
    if (kind == TY_FUNC) {
      kind = (*(node->args + i))->lhs->ty->ret_ty->kind;
    }

    switch (kind) {
      case TY_FLOAT:
      case TY_DOUBLE:
        (*floating)++;
        break;
      default:
        (*general)++;
    }
  }
}

static void push_func_params(Node *node, bool is_reg) {
  Type *ty = node->func->ty;

  int general = 0, floating = 0;
  cnt_func_params(node, &general, &floating);

  for (int i = node->argc - 1; i >= 0; i--) {
    TypeKind kind = (*(node->args + i))->lhs->ty->kind;
    if (kind == TY_FUNC) {
      kind = (*(node->args + i))->lhs->ty->ret_ty->kind;
    }

    switch (kind) {
      case TY_FLOAT:
      case TY_DOUBLE: {
        bool is_expand = false;
        floating--;
        is_expand |= (floating <= 7 && is_reg);
        is_expand |= (floating > 7 && !is_reg);

        if (is_expand) {
          compile_node((*(node->args + i))->lhs);
          println("  movd rax, xmm0");
          gen_push("rax");
        }
        break;
      }
      default: {
        bool is_expand = false;
        general--;
        is_expand |= (general <= 5 && is_reg);
        is_expand |= (general > 5 && !is_reg);

        if (is_expand) {
          compile_node((*(node->args + i))->lhs);
          gen_push("rax");
        }
      }
    }
  }
}

void compile_node(Node *node) {
  if (node->kind == ND_NUM) {
    switch (node->ty->kind) {
      case TY_CHAR:
      case TY_SHORT:
      case TY_INT:
      case TY_LONG:
        println("  mov rax, %ld", node->val);
        break;
      case TY_DOUBLE:
        println("  movsd xmm0, QWORD PTR .LC%d[rip]", node->use_var->offset);
    }
    Type *ty = node->ty;
    return;
  }

  if (node->kind == ND_CAST) {
    gen_cast(node);
    return;
  } 

  if (node->kind == ND_BLOCK) {
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
      gen_addr(node);
      gen_load(node->ty);
      return;
    case ND_ADDR:
      gen_addr(node->lhs);
      return;
    case ND_ASSIGN:
      expand_assign(node);
      return;
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
    case ND_WHILE:
    case ND_FOR: {
      int now_label = branch_label++;
      node->label = now_label;
      if (node->init) {
        compile_node(node->init);
      }

      println(".Lbegin%d:", now_label);

      // judege expr
      if (node->cond != NULL) {
        compile_node(node->cond);
        println("  cmp rax, 0");
        println("  je .Lend%d", now_label);
      }

      compile_node(node->then);

      // repeat expr
      if (node->loop != NULL) {
        compile_node(node->loop);
      }

      // finally
      println("  jmp .Lbegin%d", now_label);
      println(".Lend%d:", now_label);
      return;
    }
    case ND_LOOPBREAK: {
      int now_label = node->lhs->label;
      println("  jmp .Lend%d", now_label);
      return;
    }
    case ND_CONTINUE: {
      int now_label = node->lhs->label;
      println("  jmp .Lbegin%d", now_label);
      return;
    }
    case ND_CONTENT: {
      compile_node(node->lhs);
      gen_load(node->ty);
      return;
    }
    case ND_BITWISENOT: {
      compile_node(node->lhs);
      println("  not rax");
      return;
    }
    case ND_LOGICALNOT: {
      compile_node(node->lhs);
      if (node->lhs->ty->kind == TY_DOUBLE) {
        println("  pxor xmm2, xmm2");
        println("  ucomisd xmm2, xmm0");
        println("  setnp al");
        println("  xor rdx, rdx");
        println("  ucomisd xmm2, xmm0");
        println("  cmovne eax, edx");
        println("  movzx rax, al");
      } else {
        println("  cmp rax, 0");
        println("  sete al");
        println("  movzx rax, al");
      }
      return;
    }
    default:
      break;
  }

  if (node->kind == ND_FUNCCALL) {
    Type *ty = node->func->ty;

    push_func_params(node, false);
    push_func_params(node, true);

    int general = 0, floating = 0, stack = 0;
    for (int i = 0; i < node->argc; i++) {
      TypeKind kind = (*(node->args + i))->lhs->ty->kind;
      if (kind == TY_FUNC) {
        kind = (*(node->args + i))->lhs->ty->ret_ty->kind;
      }

      switch (kind) {
        case TY_FLOAT:
        case TY_DOUBLE:
          if (floating <= 7) {
            gen_pop("rax");
            println("  movd xmm%d, rax", floating);
          } else {
            stack++;
          }
          floating++;
          break;
        default:
          if (general <= 5) {
            gen_pop("rax");
            println("  mov %s, rax", args_reg[general]);
          } else {
            stack++;
          }
          general++;
      }
    }

    println("  call %s", node->func->name);
    if (stack > 0) {
      gen_emptypop(stack);
    }
    return;
  }

  // lhs: rax, rhs: rdi
  compile_node(node->rhs);
  if (node->rhs->ty->kind == TY_DOUBLE) {
    println("  movaps xmm1, xmm0");
  } else {
    gen_push("rax");
  }

  compile_node(node->lhs);
  if (node->rhs->ty->kind == TY_DOUBLE) {
    // no
  } else {
    gen_pop("rdi");
  }

  if (node->lhs->ty->kind == TY_DOUBLE) {
    switch (node->kind) {
      case ND_ADD:
        println("  addsd xmm0, xmm1");
        return;
      case ND_SUB:
        println("  subsd xmm0, xmm1");
        return;
      case ND_MUL:
        println("  mulsd xmm0, xmm1");
        return;
      case ND_DIV:
        println("  divsd xmm0, xmm1");
        return;
      case ND_EQ:
        println("  xor rdx, rdx");
        println("  ucomisd xmm0, xmm1");
        println("  setnp al");
        println("  ucomisd xmm0, xmm1");
        println("  cmovne eax, edx");
        println("  movzx rax, al");
        return;
      case ND_NEQ:
        println("  mov rdx, 1");
        println("  ucomisd xmm0, xmm1");
        println("  setp al");
        println("  ucomisd xmm0, xmm1");
        println("  cmovne eax, edx");
        println("  movzx rax, al");
        return;
      case ND_LC:
        println("  comisd xmm1, xmm0");
        println("  seta al");
        println("  movzx rax, al");
        return;
      case ND_LEC:
        println("  comisd xmm1, xmm0");
        println("  setnb al");
        println("  movzx rax, al");
        return;
      case ND_LOGICALAND:
        println("  pxor xmm2, xmm2");
        println("  ucomisd xmm2, xmm0");
        println("  jp 1f");
        println("  ucomisd xmm2, xmm0");
        println("  je 3f");
        println("1:");
        println("  ucomisd xmm2, xmm1");
        println("  jp 2f");
        println("  ucomisd xmm2, xmm1");
        println("  je 3f");
        println("2:");
        println("  mov rax, 1");
        println("  jmp 4f");
        println("3:");
        println("  mov rax, 0");
        println("4:");
        return;
      case ND_LOGICALOR:
        println("  pxor xmm2, xmm2");
        println("  ucomisd xmm2, xmm0");
        println("  jp 1f");
        println("  ucomisd xmm2, xmm0");
        println("  jne 1f");
        println("  ucomisd xmm2, xmm1");
        println("  jp 1f");
        println("  ucomisd xmm2, xmm1");
        println("  jne 1f");
        println("  mov rax, 0");
        println("  jmp 2f");
        println("1:");
        println("  mov rax, 1");
        println("2:");
        return;
    }
  }


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
      gen_comp("sete", rax, rdi);
      break;
    case ND_NEQ:
      gen_comp("setne", rax, rdi);
      break;
    case ND_LC: {
      bool is_unsigned = false;
      is_unsigned |= node->lhs->ty->is_unsigned;
      is_unsigned |= node->rhs->ty->is_unsigned;

      if (is_unsigned) {
        gen_comp("setb", rax, rdi);
      } else {
        gen_comp("setl", rax, rdi);
      }
      break;
    }
    case ND_LEC: {
      bool is_unsigned = false;
      is_unsigned |= node->lhs->ty->is_unsigned;
      is_unsigned |= node->rhs->ty->is_unsigned;

      if (is_unsigned) {
        gen_comp("setbe", rax, rdi);
      } else {
        gen_comp("setle", rax, rdi);
      }
      break;
    }
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
  for (Obj *gvar = get_gvars(); gvar != NULL; gvar = gvar->next) {
    // Only string literal
    switch (gvar->ty->kind) {
      case TY_STR:
      case TY_FLOAT:
      case TY_DOUBLE:
      case TY_LDOUBLE:
        gen_gvar_define(gvar);
        break;
      default:
        break;
    }
  }

  // Expand functions
  for (Node *node = head; node != NULL; node = node->next) {
    if (node->kind != ND_FUNC) {
      if (node->kind == ND_INIT) {
        gen_gvar_init(node);
      } else {
        gen_gvar_define(node->use_var);
      }
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

    // Push arguments into the stack.
    for (int i = 0; i < func->ty->param_cnt; i++) {
      if (i < 6) {
        gen_push(args_reg[i]);
      } else {
        println("  mov rax, QWORD PTR [rbp + %d]", 8 + (i - 5) * 8);
        gen_push("rax");
      }
    }

    // Set arguments
    for (int i = func->ty->param_cnt - 1; i >= 0; i--) {
      Obj *arg = *(func->params + i);

      gen_pop("rcx");
      gen_addr(new_var(NULL, arg));
      gen_push("rax");
      println("  mov rax, rcx");
      gen_store(arg->ty);
    }

    compile_node(node->deep);

    println("  mov rsp, rbp");
    gen_pop("rbp");
    println("  ret");
  }
  fclose(output_file);
}
