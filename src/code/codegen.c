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
      if (node->use_var->type->kind == TY_STR) {
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

static const char *args_reg[] = {
  "rdi", "rsi", "rdx", "rcx", "r8", "r9",
};

static char i32i8[]  = "movsx eax, al";
static char i32u8[]  = "movzx eax, al";
static char i32i16[] = "movsx eax, ax";
static char i32u16[] = "movzx eax, ax";
static char i64i32[] = "movsxd rax, eax";
static char i64u32[] = "mov eax, eax";

static char *cast_table[][8] = {
// i8     i16     i32   i64     u8     u16     u32   u64      to/from
  {NULL,  NULL,   NULL, i64i32, i32u8, i32u16, NULL, i64i32}, // i8
  {i32i8, NULL,   NULL, i64i32, i32u8, i32u16, NULL, i64i32}, // i16
  {i32i8, i32i16, NULL, i64i32, i32u8, i32u16, NULL, i64i32}, // i32
  {i32i8, i32i16, NULL, NULL,   i32u8, i32u16, NULL, NULL},   // i64

  {i32i8, NULL,   NULL, i64i32, i32u8, NULL,   NULL, i64i32}, // u8
  {i32i8, i32i16, NULL, i64i32, i32u8, NULL,   NULL, i64i32}, // u16
  {i32i8, i32i16, NULL, i64u32, i32u8, i32u16, NULL, i64u32}, // u32
  {i32i8, i32i16, NULL, NULL,   i32u8, i32u16, NULL, NULL},   // u64
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
      ret = 3;
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
  int from = get_type_idx(node->lhs->type);
  int to = get_type_idx(node->type);
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
  gen_store(node->type);
}

static void gen_logical(Node *node, int label) {
  compile_node(node->lhs);
  println("  cmp rax, 0");

  if (node->kind == ND_LOGICALAND) {
    println("  je .Lfalse%d", label);
  } else {
    println("  jne .Ltrue%d", label);
  }

  compile_node(node->rhs);
  println("  cmp rax, 0");
  println("  je .Lfalse%d", label);

  println(".Ltrue%d:", label);
  println("  mov rax, 1");
  println("  jmp .Lthen%d", label);
  println(".Lfalse%d:", label);
  println("  mov rax, 0");
  println(".Lthen%d:", label);
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

    gen_store(node->type);
    println("  mov rax, rdi");
    println("  add rax, %d", node->type->var_size);
  }
}

static void gen_gvar_init(Node *node) {
  println(".data");
  println("%s:", node->lhs->use_var->name);

  char *asm_ty;
  switch (node->type->kind) {
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

  for (Node *init = node->rhs; init != NULL; init = init->lhs) {
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
  if (var->type->kind == TY_STR) {
    println(".LC%d:", var->offset);
    println("  .string \"%s\"", var->name);
    return;
  }
  println("%s:", var->name);
  switch (var->type->kind) {
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
      println("  .zero %d", var->type->var_size);
      break;
    default:
      return;
  }
}

void compile_node(Node *node) {
  if (node->kind == ND_INT) {
    println("  mov rax, %ld", node->val);
    return;
  }

  if (node->kind == ND_CAST) {
    gen_cast(node);
    return;
  } 

  if (node->kind == ND_BLOCK) {
    for (Node *now_stmt = node->next_block; now_stmt;
         now_stmt = now_stmt->next_stmt) {
      compile_node(now_stmt);
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
      gen_load(node->type);
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
      gen_load(node->type);
      return;
    }
    case ND_LOGICALAND:
    case ND_LOGICALOR: {
      gen_logical(node, branch_label++);
      return;
    }
    case ND_BITWISENOT: {
      compile_node(node->lhs);
      println("  not rax");
      return;
    }
    case ND_LOGICALNOT: {
      compile_node(node->lhs);
      println("  cmp rax, 0");
      println("  sete al");
      println("  movzx rax, al");
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
      gen_push("rax");
      arg_count++;
    }

    for (int arg_idx = 0; arg_idx < arg_count && arg_idx < 6; arg_idx++) {
      gen_pop(args_reg[arg_idx]);
    }
    println("  call %s", name);
    if (arg_count > 6) {
      gen_emptypop(arg_count - 6);
    }
    return;
  }

  // lhs: rax, rhs: rdi
  compile_node(node->rhs);
  gen_push("rax");
  compile_node(node->lhs);
  gen_pop("rdi");

  // Default register is 32bit
  char *rax = "eax", *rdi = "edi", *rdx = "edx";

  if (node->lhs->type->kind == TY_LONG || node->lhs->type->base != NULL) {
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
      if (node->type->is_unsigned) {
        println("  mov rdx, 0");
        println("  div %s", rdi);
      } else {
        if (node->lhs->type->var_size == 8) {
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
      if (node->lhs->type->is_unsigned) {
        println("  shr %s, cl", rax);
      } else {
        println("  sar %s, cl", rax);
      }
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
    case ND_EQ:
      gen_comp("sete", rax, rdi);
      break;
    case ND_NEQ:
      gen_comp("setne", rax, rdi);
      break;
    case ND_LC:
      if (node->type->is_unsigned) {
        gen_comp("setb", rax, rdi);
      } else {
        gen_comp("setl", rax, rdi);
      }
      break;
    case ND_LEC:
      if (node->type->is_unsigned) {
        gen_comp("setbe", rax, rdi);
      } else {
        gen_comp("setle", rax, rdi);
      }
      break;
    default:
      break;
  }
}

void codegen(Node *head, char *filename) {
  output_file = fopen(filename, "w");
  println(".intel_syntax noprefix");
  for (Obj *gvar = gvars; gvar != NULL; gvar = gvar->next) {
    // Only string literal
    if (gvar->type->kind == TY_STR) {
      gen_gvar_define(gvar);
    }
  }

  // Expand functions
  for (Node *node = head; node != NULL; node = node->next_block) {
    if (node->kind != ND_FUNC) {
      for (Node *var = node; var != NULL; var = var->next_stmt) {
        if (var->kind == ND_INIT) {
          gen_gvar_init(var);
        } else {
          gen_gvar_define(var->use_var);
        }
      }
      continue;
    }
    Obj *func = node->func;
    println(".global %s", func->name);
    println(".text");
    println("%s:", func->name);

    // Prologue
    gen_push("rbp");
    println("  mov rbp, rsp");
    println("  sub rsp, %d", func->vars_size);

    // Push arguments into the stack.
    Node **args = calloc(func->argc, sizeof(Node*));
    int argc = func->argc - 1;
    for (Node *arg = func->args; arg != NULL; arg = arg->lhs) {
      if (argc < 6) {
        gen_push(args_reg[argc]);
      } else {
        println("  mov rax, QWORD PTR [rbp + %d]", 8 + (argc - 5) * 8);
        gen_push("rax");
      }
      *(args + argc) = arg;
      argc--;
    }

    // Set arguments
    argc = 0;
    while (argc < func->argc) {
      Node *arg = *(args + argc);

      gen_pop("rcx");
      gen_addr(arg);
      gen_push("rax");
      println("  mov rax, rcx");
      gen_store(arg->use_var->type);

      argc++;
    }

    compile_node(node->next_stmt);

    println("  mov rsp, rbp");
    gen_pop("rbp");
    println("  ret");
  }
  fclose(output_file);
}
