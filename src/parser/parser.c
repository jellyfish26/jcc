#include "parser/parser.h"
#include "token/tokenize.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// This structure is used to initialize variable.
// In particular, Array initializer can be nested, so need tree data structure.
typedef struct Initializer Initializer;
struct Initializer {
  Type *ty;
  Token *tkn;

  // The size of the first array can be omitted.
  bool is_flexible;
  Initializer **children;

  Node *node;
};

// Prototype
static void initializer_only(Token *tkn, Token **end_tkn, Initializer *init);
static Initializer *initializer(Token *tkn, Token **end_tkn, Type *ty);
static Node *topmost(Token *tkn, Token **end_tkn);
static Node *statement(Token *tkn, Token **end_tkn, bool new_scope);
static Node *declarations(Token *tkn, Token**end_tkn, bool is_global);
static Node *assign(Token *tkn, Token **end_tkn);
static Node *ternary(Token *tkn, Token **end_tkn);
static Node *logical_or(Token *tkn, Token **end_tkn);
static Node *logical_and(Token *tkn, Token **end_tkn);
static Node *bitwise_or(Token *tkn, Token **end_tkn);
static Node *bitwise_xor(Token *tkn, Token **end_tkn);
static Node *bitwise_and(Token *tkn, Token **end_tkn);
static Node *same_comp(Token *tkn, Token **end_tkn);
static Node *size_comp(Token *tkn, Token **end_tkn);
static Node *bitwise_shift(Token *tkn, Token **end_tkn);
static Node *add(Token *tkn, Token **end_tkn);
static Node *mul(Token *tkn, Token **end_tkn);
static Node *cast(Token *tkn, Token **end_tkn);
static Node *unary(Token *tkn, Token **end_tkn);
static Node *address_op(Token *tkn, Token **end_tkn);
static Node *indirection(Token *tkn, Token **end_tkn);
static Node *increment_and_decrement(Token *tkn, Token **end_tkn);
static Node *priority(Token *tkn, Token **end_tkn);
static Node *num(Token *tkn, Token **end_tkn);


Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *ret = calloc(1, sizeof(Node));
  ret->kind = kind;
  ret->lhs = lhs;
  ret->rhs = rhs;
  return ret;
}

Node *new_num(int val) {
  Node *ret = calloc(1, sizeof(Node));
  ret->kind = ND_INT;
  ret->val = val;
  ret->type = new_type(TY_INT, false);
  return ret;
}

Node *new_cast(Node *lhs, Type *type) {
  Node *ret = new_node(ND_CAST, NULL, NULL);
  ret->lhs = lhs;
  add_type(ret);
  ret->type = type;
  return ret;
}

Node *new_var(Obj *obj) {
  Node *ret = new_node(ND_VAR, NULL, NULL);
  ret->use_var = obj;
  ret->type = obj->type;
  return ret;
}

// kind is ND_ASSIGN if operation only assign
static Node *new_assign(NodeKind kind, Node *lhs, Node *rhs) {
  Node *ret = new_node(ND_ASSIGN, lhs, rhs);
  ret->assign_type = kind;
  return ret;
}

char *typename[] = {
  "void", "char", "short", "int", "long"
};

static bool is_typename(Token *tkn) {
  for (int i = 0; i < sizeof(typename) / sizeof(char *); i++) {
    if (equal(tkn, typename[i])) {
      return true;
    }
  }
  return false;
}

// If not ident, return NULL.
static char *get_ident(Token *tkn) {
  if (tkn->kind != TK_IDENT) {
    return NULL;
  }
  char *ret = calloc(tkn->len + 1, sizeof(char));
  memcpy(ret, tkn->loc, tkn->len);
  return ret;
}

static Type *get_type(Token *tkn, Token **end_tkn) {
  // We replace the type with a number and count it,
  // which makes it easier to detect duplicates and types.
  // If typename is 'long', we have a duplicate when the long bit
  // and the high bit are 1.
  // Otherwise, we have a duplicate when the high is 1.
  enum {
    VOID  = 1 << 0,
    CHAR  = 1 << 2,
    SHORT = 1 << 4,
    INT   = 1 << 6,
    LONG  = 1 << 8,
  };
  
  int type_cnt = 0;
  Type *ret = NULL;
  while (is_typename(tkn)) {

    // Counting Types
    if (equal(tkn,"void")) {
      type_cnt += VOID;
    } else if (equal(tkn, "char")) {
      type_cnt += CHAR;
    } else if (equal(tkn, "short")) {
      type_cnt += SHORT;
    } else if (equal(tkn, "int")) {
      type_cnt += INT;
    } else if (equal(tkn, "long")) {
      type_cnt += LONG;
    }

    // Detect duplicates
    char *dup_type = NULL;

    // Avoid check long (so, range -1)
    for (int i = 0; i < (sizeof(typename) / sizeof(char *)) - 1; i++) {
      if (((type_cnt>>(i * 2 + 1))&1) == 1) {
        dup_type = typename[i];
      }
    }
    // Check long
    if (((LONG * 3)&type_cnt) == (LONG * 3)) {
      dup_type = "long";
    }

    if (dup_type != NULL) {
      errorf_tkn(ER_COMPILE, tkn, "Duplicate declaration of '%s'", dup_type);
    }

    switch (type_cnt) {
      case VOID:
        ret = new_type(TY_VOID, false);
        break;
      case CHAR:
        ret = new_type(TY_CHAR, true);
        break;
      case SHORT:
      case SHORT + INT:
        ret = new_type(TY_SHORT, true);
        break;
      case INT:
        ret = new_type(TY_INT, true);
        break;
      case LONG:
      case LONG + INT:
      case LONG + LONG:
      case LONG + LONG + INT:
        ret = new_type(TY_LONG, true);
        break;
      default:
        errorf_tkn(ER_COMPILE, tkn, "Invalid type");
    }
    tkn = tkn->next;
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

static Initializer *new_initializer(Type *ty, bool is_flexible) {
  Initializer *ret = calloc(1, sizeof(Initializer));
  ret->ty = ty;

  if (ty->kind == TY_ARRAY) {
    if (is_flexible && ty->var_size == 0) {
      ret->is_flexible = true;
      return ret;
    }

    ret->children = calloc(1, sizeof(Initializer *));
    for (int i = 0; i < ty->array_len; i++) {
      ret->children[i] = new_initializer(ty->base, false);
    }
    return ret;
  }
  return ret;
}

static int count_array_init_elements(Token *tkn, Type *ty) {
  int cnt = 0;
  Initializer *dummy = new_initializer(ty->base, false);
  tkn = skip(tkn, "{");

  while (true) {
    if (cnt > 0 && !consume(tkn, &tkn, ",")) break;

    if (equal(tkn, "}")) break;
    initializer_only(tkn, &tkn, dummy);
    cnt++;
  }
  return cnt;
}

// array_initializer = "{" initializer_only ("," initializer_only)* ","? "}"
static void array_initializer(Token *tkn, Token **end_tkn, Initializer *init) {
  if (init->is_flexible) {
    int len = count_array_init_elements(tkn, init->ty);
    *init = *new_initializer(array_to(init->ty->base, len), false);
  }
  tkn = skip(tkn, "{");

  int idx = 0;
  while (true) {
    if (idx > 0 && !consume(tkn, &tkn, ",")) break;

    if (consume(tkn, &tkn, "}")) break;
    initializer_only(tkn, &tkn, init->children[idx]);
    idx++;
  }
  consume(tkn, &tkn, "}");
  if (end_tkn != NULL) *end_tkn = tkn;
}

// initializer = array_initializer | assign
static void initializer_only(Token *tkn, Token **end_tkn, Initializer *init) {
  if (init->ty->kind == TY_ARRAY) {
    array_initializer(tkn, &tkn, init);
    if (end_tkn != NULL) *end_tkn = tkn;
    return;
  }

  init->node = assign(tkn, &tkn);
  if (end_tkn != NULL) *end_tkn = tkn;
}

static Initializer *initializer(Token *tkn, Token **end_tkn, Type *ty) {
  Initializer *ret = new_initializer(ty, true);
  initializer_only(tkn, &tkn, ret);
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

static Node *create_lvar_init_node(Initializer *init, Node **end_node) {
  Node *head = calloc(1, sizeof(Node));
  if (init->children == NULL) {
    Node *tmp = calloc(1, sizeof(Node));

    tmp->kind = ND_INIT;
    tmp->init = init->node;
    head->lhs = tmp;

    if (end_node != NULL) *end_node = head->lhs;
  } else {
    Node *now = head;

    for (int i = 0; i < init->ty->array_len; i++) {
      now->lhs = create_lvar_init_node(init->children[i], &now);
    }

    if (end_node != NULL) *end_node = now;
  }
  return head->lhs;
}

// pointers = ("*")*
static Type *pointers(Token *tkn, Token **end_tkn, Type *ty) {
  while (consume(tkn, &tkn, "*")) {
    ty = pointer_to(ty);
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ty;
}

// arrays = "[" num "]" arrays?
// num can empty value if empty_size is true.
static Type *arrays(Token *tkn, Token **end_tkn, Type *ty, bool empty_size) {
  if (!consume(tkn, &tkn, "[")) {
    return ty;
  }
  int val = 0;
  if (!empty_size && tkn->kind != TK_NUM_INT) {
      errorf_tkn(ER_COMPILE, tkn, "Specify the size of array.");
  }
  if (tkn->kind == TK_NUM_INT) {
    val = tkn->val;
    tkn = tkn->next;
  }
  tkn = skip(tkn, "]");
  ty = arrays(tkn, &tkn, ty, false);

  if (end_tkn != NULL) *end_tkn = tkn;
  return array_to(ty, val);
}

// declare = get_type pointers? ident arrays?
static Obj *declare(Token *tkn, Token **end_tkn, Type *ty, Type **base_ty) {
  ty = pointers(tkn, &tkn, ty);
  char *ident = get_ident(tkn);

  if (ident == NULL) {
    errorf_tkn(ER_COMPILE, tkn, "The variable declaration requires an identifier.");
  }
  
  tkn = tkn->next;

  if (base_ty != NULL) {
    *base_ty = calloc(1, sizeof(Type));
    memcpy(*base_ty, ty, sizeof(Type));
  }
  
  ty = arrays(tkn, &tkn, ty, true);

  if (end_tkn != NULL) *end_tkn = tkn;
  return new_obj(ty, ident);
}

// declarator = declare ("=" initializer)?
// Return NULL if cannot be declared.
static Node *declarator(Token *tkn, Token **end_tkn, Type *ty, bool is_global) {
  Type *base_ty;
  Obj *var = declare(tkn, &tkn, ty, &base_ty);

  if (is_global) {
    add_gvar(var);
  } else {
    add_lvar(var);
  }

  Node *ret = new_var(var);

  if (ty->var_size == 0 && !equal(tkn, "=")) {
    errorf_tkn(ER_COMPILE, tkn, "Size empty array require an initializer.");
  }

  if (consume(tkn, &tkn, "=")) {
    Initializer *init = initializer(tkn, &tkn, var->type);
    ret = new_node(ND_INIT, ret, create_lvar_init_node(init, NULL));
    ret->type = base_ty;
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

static Node *last_stmt(Node *now) {
  while (now->next_stmt != NULL) {
    now = now->next_stmt;
  }
  return now;
}

Node *program(Token *tkn) {
  lvars = NULL;
  gvars = NULL;
  used_vars = NULL;
  Node *head = calloc(1, sizeof(Node));
  Node *now = head;
  while (!is_eof(tkn)) {
    now->lhs = topmost(tkn, &tkn);
    now = now->lhs;
  }
  return head->lhs;
}

// topmost = get_type ident "(" params?")" statement |
//           declarations ";"
// params = get_type ident ("," get_type ident)*
static Node *topmost(Token *tkn, Token **end_tkn) {
  Token *head_tkn = tkn;
  Type *ty = get_type(tkn, &tkn);

  if (ty == NULL) {
    errorf_tkn(ER_COMPILE, tkn, "Undefined type.");
  }

  char *ident = get_ident(tkn);
  if (ident == NULL) {
    errorf_tkn(ER_COMPILE, tkn,
        "Identifiers are required for function and variable definition.");
  }

  tkn = tkn->next;

  // Global variable definition
  if (!consume(tkn, &tkn, "(")) {
    Node *ret = declarations(head_tkn, &tkn, true);

    if (!consume(tkn, &tkn, ";")) {
      errorf_tkn(ER_COMPILE, tkn, "A ';' is required at the end of the definition.");
    }

    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  // Fuction definition
  Node *ret = new_node(ND_FUNC, NULL, NULL);
  Obj *func = new_obj(ty, ident);
  ret->func = func;
  new_scope_definition();

  bool end_define = false;
  if (consume(tkn, &tkn, ")")) {
    end_define = true;
  }

  // Set arguments
  while (!end_define) {
    Type *arg_type = get_type(tkn, &tkn);
    if (arg_type == NULL) {
      errorf_tkn(ER_COMPILE, tkn, "Undefined type.");
    }
    
    ident = get_ident(tkn);
    if (ident == NULL) {
      errorf_tkn(ER_COMPILE, tkn, "Identifiers are required for variable definition.");
    }
    if (find_var(ident) != NULL) {
      errorf_tkn(ER_COMPILE, tkn, "This variable already define.");
    }
    tkn = tkn->next;
    Obj *lvar = new_obj(arg_type, ident);
    add_lvar(lvar);
    Node *arg = new_var(lvar);
    arg->lhs = func->args;
    func->args = arg;
    func->args->is_var_define_only = true;
    func->argc++;

    if (consume(tkn, &tkn, ",")) {
      continue;
    }

    if (consume(tkn, &tkn, ")")) {
      break;
    } else {
      errorf_tkn(ER_COMPILE, tkn, "Define function must tend with \")\".");
    }
  }

  ret->next_stmt = statement(tkn, &tkn, false);
  out_scope_definition();
  func->vars_size = init_offset();

  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

static Node *inside_roop; // inside for or while

// statement = { statement* } |
//             ("return")? assign ";" |
//             "if" "(" assign ")" statement ("else" statement)? |
//             "for" "(" declarations? ";" assign? ";" assign?")" statement |
//             "while" "(" assign ")" statement |
//             "break;" |
//             "continue;" |
//             declarations ";"
static Node *statement(Token *tkn, Token **end_tkn, bool new_scope) {
  Node *ret;

  // Block statement
  if (consume(tkn, &tkn, "{")) {
    if (new_scope) {
      new_scope_definition();
    }
    ret = new_node(ND_BLOCK, NULL, NULL);
    Node *head = calloc(1, sizeof(Node));
    Node *now = head;
    while (!consume(tkn, &tkn, "}")) {
      now->next_stmt = statement(tkn, &tkn, true);
      now = last_stmt(now);
    }
    ret->next_block = head->next_stmt;
    if (new_scope) {
      out_scope_definition();
    }
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  if (consume(tkn, &tkn, "if")) {
    new_scope_definition();
    ret = new_node(ND_IF, NULL, NULL);

    if (!consume(tkn, &tkn, "(")) {
      errorf_tkn(ER_COMPILE, tkn, "If statement must start with \"(\"");
    }
    ret->judge = assign(tkn, &tkn);
    Obj *top_var = lvars->objs;
    if (!consume(tkn, &tkn, ")")) {
      errorf_tkn(ER_COMPILE, tkn, "If statement must end with \")\".");
    }
    ret->exec_if = statement(tkn, &tkn, false);
    // Transfer varaibles
    if (top_var && lvars->objs == top_var) {
      lvars->objs = NULL;
    } else if (top_var) {
      for (Obj *obj = lvars->objs; obj != NULL; obj = obj->next) {
        if (obj->next == top_var) {
          obj->next = NULL;
          break;
        }
      }
    }
    out_scope_definition();
    if (consume(tkn, &tkn, "else")) {
      new_scope_definition();
      if (top_var) {
        add_lvar(top_var);
      }
      ret->exec_else = statement(tkn, &tkn, false);
      out_scope_definition();
    }
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  if (consume(tkn, &tkn, "for")) {
    new_scope_definition();
    Node *roop_state = inside_roop;
    ret = new_node(ND_FOR, NULL, NULL);
    inside_roop = ret;

    if (!consume(tkn, &tkn, "(")) {
      errorf_tkn(ER_COMPILE, tkn, "For statement must start with \"(\".");
    }

    if (!consume(tkn, &tkn, ";")) {
      ret->init = declarations(tkn, &tkn, false);
      if (!consume(tkn, &tkn, ";")) {
        errorf_tkn(ER_COMPILE, tkn, "After defining an expression, it must end with \";\". ");
      }
    }

    if (!consume(tkn, &tkn, ";")) {
      ret->judge = assign(tkn, &tkn);
      if (!consume(tkn, &tkn, ";")) {
        errorf_tkn(ER_COMPILE, tkn, "After defining an expression, it must end with \";\". ");
      }
    }

    if (!consume(tkn, &tkn, ")")) {
      ret->repeat_for = assign(tkn, &tkn);
      if (!consume(tkn, &tkn, ")")) {
        errorf_tkn(ER_COMPILE, tkn, "For statement must end with \")\".");
      }
    }

    ret->stmt_for = statement(tkn, &tkn, false);
    out_scope_definition();

    inside_roop = roop_state;
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  if (consume(tkn, &tkn, "while")) {
    new_scope_definition();
    Node *roop_state = inside_roop;
    ret = new_node(ND_WHILE, NULL, NULL);
    inside_roop = ret;

    if (!consume(tkn, &tkn, "(")) {
      errorf_tkn(ER_COMPILE, tkn, "While statement must start with \"(\".");
    }

    ret->judge = assign(tkn, &tkn);

    if (!consume(tkn, &tkn, ")")) {
      errorf_tkn(ER_COMPILE, tkn, "While statement must end with \")\".");
    }

    ret->stmt_for = statement(tkn, &tkn, false);
    inside_roop = roop_state;
    out_scope_definition();

    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  if (consume(tkn, &tkn, "return")) {
    ret = new_node(ND_RETURN, assign(tkn, &tkn), NULL);

    if (!consume(tkn, &tkn, ";")) {
      errorf_tkn(ER_COMPILE, tkn, "Expression must end with \";\".");
    }

    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  if (consume(tkn, &tkn, "break")) {
    if (!inside_roop) {
      errorf_tkn(ER_COMPILE, tkn, "%s", "Not whithin loop");
    }

    ret = new_node(ND_LOOPBREAK, NULL, NULL);
    ret->lhs = inside_roop;

    if (!consume(tkn, &tkn, ";")) {
      errorf_tkn(ER_COMPILE, tkn, "Expression must end with \";\".");
    }

    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  if (consume(tkn, &tkn, "continue")) {
    if (!inside_roop) {
      errorf_tkn(ER_COMPILE, tkn, "%s", "continue statement not within loop");
    }
    ret = new_node(ND_CONTINUE, NULL, NULL);
    ret->lhs = inside_roop;
    if (!consume(tkn, &tkn, ";")) {
      errorf_tkn(ER_COMPILE, tkn, "Expression must end with \";\".");
    }
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  ret = declarations(tkn, &tkn, false);
  if (!consume(tkn, &tkn, ";")) {
    errorf_tkn(ER_COMPILE, tkn, "Expression must end with \";\".");
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// declarations = get_type declarator ("," declarator)* |
//                assign
// Return last node, head is connect node.
static Node *declarations(Token *tkn, Token**end_tkn, bool is_global) {
  Type *ty = get_type(tkn, &tkn);
  if (ty == NULL) {
    Node *ret = assign(tkn, &tkn);
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  Node *ret = declarator(tkn, &tkn, ty, is_global);
  Node *now = ret;
  while (consume(tkn, &tkn, ",")) {
    now->next_stmt = declarator(tkn, &tkn, ty, is_global);
    now = now->next_stmt;
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}


// assign = ternary ("=" assign | "+=" assign | "-=" assign
//                   "*=" assign | "/=" assign | "%=" assign
//                   "<<=" assign | ">>=" assign
//                   "&=" assign | "^=" assign | "|=" assign)?
static Node *assign(Token *tkn, Token **end_tkn) {
  Node *ret = ternary(tkn, &tkn);

  if (consume(tkn, &tkn, "=")) {
    ret = new_assign(ND_ASSIGN, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, "+=")) {
    ret = new_assign(ND_ADD, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, "-=")) {
    ret = new_assign(ND_SUB, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, "*=")) {
    ret = new_assign(ND_MUL, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, "/=")) {
    ret = new_assign(ND_DIV, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, "%=")) {
    ret = new_assign(ND_REMAINDER, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, "<<=")) {
    ret = new_assign(ND_LEFTSHIFT, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, ">>=")) {
    ret = new_assign(ND_RIGHTSHIFT, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, "&=")) {
    ret = new_assign(ND_BITWISEAND, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, "^=")) {
    ret = new_assign(ND_BITWISEXOR, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, "|=")) {
    ret = new_assign(ND_BITWISEOR, ret, assign(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  add_type(ret);
  return ret;
}

// ternary = logical_or ("?" ternary ":" ternary)?
static Node *ternary(Token *tkn, Token **end_tkn) {
  Node *ret = logical_or(tkn, &tkn);

  if (consume(tkn, &tkn, "?")) {
    Node *tmp = new_node(ND_TERNARY, NULL, NULL);
    tmp->lhs = ternary(tkn, &tkn);
    if (!consume(tkn, &tkn, ":")) {
      errorf_tkn(ER_COMPILE, tkn, "The ternary operator requires \":\".");
    }
    tmp->rhs = ternary(tkn, &tkn);
    tmp->exec_if = ret;
    ret = tmp;
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// logical_or = logical_and ("||" logical_or)?
static Node *logical_or(Token *tkn, Token **end_tkn) {
  Node *ret = logical_and(tkn, &tkn);

  if (consume(tkn, &tkn, "||")) {
    ret = new_node(ND_LOGICALOR, ret, logical_or(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// logical_and = bitwise_or ("&&" logical_and)?
static Node *logical_and(Token *tkn, Token **end_tkn) {
  Node *ret = bitwise_or(tkn, &tkn);

  if (consume(tkn, &tkn, "&&")) {
    ret = new_node(ND_LOGICALAND, ret, logical_and(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// bitwise_or = bitwise_xor ("|" bitwise_or)?
static Node *bitwise_or(Token *tkn, Token **end_tkn) {
  Node *ret = bitwise_xor(tkn, &tkn);
  if (consume(tkn, &tkn, "|")) {
    ret = new_node(ND_BITWISEOR, ret, bitwise_or(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// bitwise_xor = bitwise_and ("^" bitwise_xor)?
static Node *bitwise_xor(Token *tkn, Token **end_tkn) {
  Node *ret = bitwise_and(tkn, &tkn);
  if (consume(tkn, &tkn, "^")) {
    ret = new_node(ND_BITWISEXOR, ret, bitwise_xor(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// bitwise_and = same_comp ("&" bitwise_and)?
static Node *bitwise_and(Token *tkn, Token **end_tkn) {
  Node *ret = same_comp(tkn, &tkn);
  if (consume(tkn, &tkn, "&")) {
    ret = new_node(ND_BITWISEAND, ret, bitwise_and(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// same_comp = size_comp ("==" same_comp | "!=" sama_comp)?
static Node *same_comp(Token *tkn, Token **end_tkn) {
  Node *ret = size_comp(tkn, &tkn);
  if (consume(tkn, &tkn, "==")) {
    ret = new_node(ND_EQ, ret, same_comp(tkn, &tkn));
  } else if (consume(tkn, &tkn, "!=")) {
    ret = new_node(ND_NEQ, ret, same_comp(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// size_comp = bitwise_shift ("<" size_comp  | ">" size_comp | "<=" size_comp |
// ">=" size_comp)?
static Node *size_comp(Token *tkn, Token **end_tkn) {
  Node *ret = bitwise_shift(tkn, &tkn);
  if (consume(tkn, &tkn, "<")) {
    ret = new_node(ND_LC, ret, size_comp(tkn, &tkn));
  } else if (consume(tkn, &tkn, ">")) {
    ret = new_node(ND_RC, ret, size_comp(tkn, &tkn));
  } else if (consume(tkn, &tkn, "<=")) {
    ret = new_node(ND_LEC, ret, size_comp(tkn, &tkn));
  } else if (consume(tkn, &tkn, ">=")) {
    ret = new_node(ND_REC, ret, size_comp(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// bitwise_shift = add ("<<" bitwise_shift | ">>" bitwise_shift)?
static Node *bitwise_shift(Token *tkn, Token **end_tkn) {
  Node *ret = add(tkn, &tkn);
  if (consume(tkn, &tkn, "<<")) {
    ret = new_node(ND_LEFTSHIFT, ret, bitwise_shift(tkn, &tkn));
  } else if (consume(tkn, &tkn, ">>")) {
    ret = new_node(ND_RIGHTSHIFT, ret, bitwise_shift(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// add = mul ("+" add | "-" add)?
static Node *add(Token *tkn, Token **end_tkn) {
  Node *ret = mul(tkn, &tkn);
  if (consume(tkn, &tkn, "+")) {
    ret = new_node(ND_ADD, ret, add(tkn, &tkn));
  } else if (consume(tkn, &tkn, "-")) {
    ret = new_node(ND_SUB, ret, add(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// mul =  cast ("*" mul | "/" mul | "%" mul)?
static Node *mul(Token *tkn, Token **end_tkn) {
  Node *ret = cast(tkn, &tkn);
  if (consume(tkn, &tkn, "*")) {
    ret = new_node(ND_MUL, ret, mul(tkn, &tkn));
  } else if (consume(tkn, &tkn, "/")) {
    ret = new_node(ND_DIV, ret, mul(tkn, &tkn));
  } else if (consume(tkn, &tkn, "%")) {
    ret = new_node(ND_REMAINDER, ret, mul(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// cast = ("(" get_type pointers? ")") cast |
//        unary
static Node *cast(Token *tkn, Token **end_tkn) {
  if (equal(tkn, "(")) {
    if (get_type(tkn->next, NULL) == NULL) {
      Node *ret = unary(tkn, &tkn);
      if (end_tkn != NULL) *end_tkn = tkn;
      return ret;
    }
    Type *ty = get_type(tkn->next, &tkn);
    ty = pointers(tkn, &tkn, ty);
    if (!consume(tkn, &tkn, ")")) {
      errorf_tkn(ER_COMPILE, tkn, "Cast must end with \")\".");
    }
    Node *ret = new_cast(cast(tkn, &tkn), ty);
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }
  Node *ret = unary(tkn, &tkn);
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// unary = "sizeof" unary
//         ("+" | "-" | "!" | "~")? address_op
static Node *unary(Token *tkn, Token **end_tkn) {
  if (consume(tkn, &tkn, "sizeof")) {
    Node *ret = new_node(ND_SIZEOF, unary(tkn, &tkn), NULL);
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }
  Node *ret = NULL;
  if (consume(tkn, &tkn, "+")) {
    ret = new_node(ND_ADD, new_num(0), address_op(tkn, &tkn));
  } else if (consume(tkn, &tkn, "-")) {
    ret = new_node(ND_SUB, new_num(0), address_op(tkn, &tkn));
  } else if (consume(tkn, &tkn, "!")) {
    ret = new_node(ND_LOGICALNOT, address_op(tkn, &tkn), NULL);
  } else if (consume(tkn, &tkn, "~")) {
    ret = new_node(ND_BITWISENOT, address_op(tkn, &tkn), NULL);
  }
  if (ret == NULL) {
    ret = address_op(tkn, &tkn);
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// address_op = "&"? indirection
static Node *address_op(Token *tkn, Token **end_tkn) {
  Node *ret = NULL;
  if (consume(tkn, &tkn, "&")) {
    ret = new_node(ND_ADDR, indirection(tkn, &tkn), NULL);
  }
  if (ret == NULL) {
    ret = indirection(tkn, &tkn);
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// indirection = (increment_and_decrement | "*" indirection)
static Node *indirection(Token *tkn, Token **end_tkn) {
  Node *ret = NULL;
  if (consume(tkn, &tkn, "*")) {
    ret = new_node(ND_CONTENT, indirection(tkn, &tkn), NULL);
  }
  if (ret == NULL) {
    ret = increment_and_decrement(tkn, &tkn);
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// increment_and_decrement = priority |
//                           ("++" | "--") priority |
//                           priority ("++" | "--")
static Node *increment_and_decrement(Token *tkn, Token **end_tkn) {
  Node *ret = NULL;
  if (consume(tkn, &tkn, "++")) {
    ret = new_node(ND_PREFIX_INC, priority(tkn, &tkn), NULL);
  } else if (consume(tkn, &tkn, "--")) {
    ret = new_node(ND_PREFIX_INC, priority(tkn, &tkn), NULL);
  }

  if (ret != NULL) {
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  ret = priority(tkn, &tkn);
  if (consume(tkn, &tkn, "++")) {
    ret = new_node(ND_SUFFIX_INC, ret, NULL);
  } else if (consume(tkn, &tkn, "--")) {
    ret = new_node(ND_SUFFIX_DEC, ret, NULL);
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// priority = num |
//            "({" statement statement* "})" |
//            String(literal) |
//            "(" assign ")" |
//            ident ("(" params? ")")? |
//            ident ("[" assign "]")* |
// params = assign ("," assign)?
// base_type is gen_type()
static Node *priority(Token *tkn, Token **end_tkn) {
  if (consume(tkn, &tkn, "(")) {
    // GNU Statements and Declarations
    if (equal(tkn, "{")) {
      Node *ret = statement(tkn, &tkn, true);
      if (!consume(tkn, &tkn, ")")) {
        errorf_tkn(ER_COMPILE, tkn, "\"(\" and \")\" should be written in pairs.");
      }
      if (end_tkn != NULL) *end_tkn = tkn;
      return ret;
    }
    Node *ret = assign(tkn, &tkn);

    if (!consume(tkn, &tkn, ")")) {
      errorf_tkn(ER_COMPILE, tkn, "\"(\" and \")\" should be written in pairs.");
    }
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  // String literal
  if (tkn->kind == TK_STR) {
    Obj *var = new_obj(new_type(TY_STR, false), tkn->str_lit);
    tkn = tkn->next;
    Node *ret = new_var(var);
    ret->is_var_define_only = false;
    add_gvar(var);
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  char *ident = get_ident(tkn);
  if (ident != NULL) {
    // function call
    tkn = tkn->next;
    if (consume(tkn, &tkn, "(")) {
      Node *ret = new_node(ND_FUNCCALL, NULL, NULL);
      ret->func = new_obj(new_type(TY_INT, false), ident); // (Warn: Temporary type)
      ret->type = ret->func->type;

      if (consume(tkn, &tkn, ")")) {
        if (*end_tkn != NULL) *end_tkn = tkn;
        return ret;
      }

      int argc = 0;
      ret->func->args = NULL;
      while (true) {
        Node *tmp = assign(tkn, &tkn);
        tmp->next_stmt = ret->func->args;
        ret->func->args = tmp;
        argc++;

        if (consume(tkn, &tkn, ",")) {
          continue;
        }

        if (!consume(tkn, &tkn, ")")) {
          errorf_tkn(ER_COMPILE, tkn, "Function call must end with \")\".");
        }
        break;
      }
      ret->func->argc = argc;
      if (end_tkn != NULL) *end_tkn = tkn;
      return ret;
    } else {
      // use variable
      Obj *use_var = find_var(ident);
      if (use_var == NULL) {
        errorf_tkn(ER_COMPILE, tkn, "This variable is not definition.");
      }
      Node *ret = new_var(use_var);
      ret->is_var_define_only = false;
      while (consume(tkn, &tkn, "[")) {
        ret = new_node(ND_ADD, ret, assign(tkn, &tkn));

        ret = new_node(ND_CONTENT, ret, NULL);
        if (!consume(tkn, &tkn, "]")) {
          errorf_tkn(ER_COMPILE, tkn, "Must use \"[\".");
        }
      }
      if (end_tkn != NULL) *end_tkn = tkn;
      return ret;
    }
  }

  Node *ret = num(tkn, &tkn);
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

static Node *num(Token *tkn, Token **end_tkn) {
  if (tkn->kind == TK_NUM_INT) {
    if (end_tkn != NULL) *end_tkn = tkn->next;
    return new_num(tkn->val);
  }
  if (tkn->kind != TK_CHAR) {
    errorf_tkn(ER_COMPILE, tkn, "Not value.");
  }
  if (end_tkn != NULL) *end_tkn = tkn->next;
  return new_num(tkn->c_lit);
}
