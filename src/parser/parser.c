#include "parser/parser.h"
#include "token/tokenize.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Prototype
static bool function(Function *func, Token *tkn, Token **end_tkn);
static Node *statement(Token *tkn, Token **end_tkn, bool new_scope);
static Node *define_var(Token *tkn, Token **end_tkn, bool once, bool is_global);
static Node *define_ident(Token *tkn, Token **end_tkn, Type *define_ident, bool is_global);
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

static Type *get_type(Token *tkn, Token **end_tkn) {
  if (consume(tkn, &tkn, "char")) {
    if (end_tkn != NULL) *end_tkn = tkn;
    return new_type(TY_CHAR, true);
  }
  if (consume(tkn, &tkn, "short")) {
    if (end_tkn != NULL) *end_tkn = tkn;
    return new_type(TY_SHORT, true);
  }
  if (consume(tkn, &tkn, "int")) {
    if (end_tkn != NULL) *end_tkn = tkn;
    return new_type(TY_INT, true);
  }
  if (consume(tkn, &tkn, "long")) {
    while (consume(tkn, &tkn, "long"));
    consume(tkn, &tkn, "int");
    if (end_tkn != NULL) *end_tkn = tkn;
    return new_type(TY_LONG, true);
  }
  return NULL;
}

static Node *last_stmt(Node *now) {
  while (now->next_stmt) {
    now = now->next_stmt;
  }
  return now;
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


Function *program(Token *tkn) {
  lvars = NULL;
  gvars = NULL;
  used_vars = NULL;
  Function head;
  Function *end = NULL;
  while (!is_eof(tkn)) {
    if (end != NULL) {
      end->next = calloc(1, sizeof(Function));
      if (function(end->next, tkn, &tkn)) {
        end = end->next;
      } else {
        end->next = NULL;
      }
    } else {
      end = calloc(1, sizeof(Function));
      if (function(end, tkn, &tkn)) {
        head.next = end;
      } else {
        end = NULL;
      }
    }
  }
  return head.next;
}


// function = base_type ident "(" params?")" statement
// params = base_type ident ("," base_type ident)*
// base_type is gen_type()
static bool function(Function *func, Token *tkn, Token **end_tkn) {
  Token *type_tkn = tkn;
  Type *ret_type = get_type(tkn, &tkn);
  if (ret_type) {
    func->ret_type = ret_type;
  } else {
    errorf_tkn(ER_COMPILE, tkn, "Undefined type.");
  }

  // Global variable define
  char *global_ident = get_ident(tkn);
  if (global_ident != NULL) {
    tkn = tkn->next;
    if (!consume(tkn, &tkn, "(")) {
      define_var(type_tkn, &tkn, false, true);
      consume(tkn, &tkn, ";");
      if (end_tkn != NULL) *end_tkn = tkn;
      return false;
    }

    // Function define
    new_scope_definition();
    bool is_var_defined = true;
    if (consume(tkn, &tkn, ")")) {
      is_var_defined = false;
    }

    // Set arguments
    while (is_var_defined) {
      Type *arg_type = get_type(tkn, &tkn);
      if (!arg_type) {
        errorf_tkn(ER_COMPILE, tkn, "Undefined type.");
      }

      // Variable define in function arguments
      char *define_ident = get_ident(tkn);
      if (define_ident != NULL) {
        Token *ident_tkn = tkn;
        tkn = tkn->next;
        if (find_var(define_ident) != NULL) {
          errorf_tkn(ER_COMPILE, ident_tkn, "This variable already definition.");
        }
        Obj *lvar = new_obj(arg_type, define_ident);
        add_lvar(lvar);
        Node *arg = new_var(lvar);
        arg->lhs = func->func_args;
        func->func_args = arg;
        func->func_args->is_var_define_only = true;
        func->func_argc++;
      } else {
        errorf_tkn(ER_COMPILE, tkn, "Must declare variable.");
      }

      if (consume(tkn, &tkn, ",")) {
        continue;
      }

      if (consume(tkn, &tkn, ")")) {
        break;
      } else {
        errorf_tkn(ER_COMPILE, tkn, "Define function must tend with \")\".");
      }
    }
    func->func_name = global_ident;
    func->func_name_len = strlen(global_ident);
    func->stmt = statement(tkn, &tkn, false);
    out_scope_definition();
    func->vars_size = init_offset();
  } else {
    errorf_tkn(ER_COMPILE, tkn, "Start the function with an identifier.");
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return true;
}

static Node *inside_roop; // inside for or while

// statement = { statement* } |
//             ("return")? assign ";" |
//             "if" "(" define_var(true, NULL) ")" statement ("else" statement)?
//             | "for" "(" define_var(true, NULL)? ";" assign? ";" assign?")"
//             statement | "while" "(" define_var(true, NULL) ")" statement |
//             "break;" |
//             "continue;" |
//             define_var(false) ";"
static Node *statement(Token *tkn, Token **end_tkn, bool new_scope) {
  Node *ret;

  // Block statement
  if (consume(tkn, &tkn, "{")) {
    if (new_scope) {
      new_scope_definition();
    }
    ret = new_node(ND_BLOCK, NULL, NULL);
    Node *now = NULL;
    while (!consume(tkn, &tkn, "}")) {
      if (now) {
        now->next_stmt = statement(tkn, &tkn, true);
      } else {
        now = statement(tkn, &tkn, true);
        ret->next_block = now;
      }
      now = last_stmt(now);
    }
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
    ret->judge = define_var(tkn, &tkn, true, false);
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
      ret->init_for = define_var(tkn, &tkn, true, false);
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

  ret = define_var(tkn, &tkn, false, false);
  if (!consume(tkn, &tkn, ";")) {
    errorf_tkn(ER_COMPILE, tkn, "Expression must end with \";\".");
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// define_var = base_type define_ident (once is false: ("," define_ident)*) |
//              assign
static Node *define_var(Token *tkn, Token **end_tkn, bool once, bool is_global) {
  Type *var_type = get_type(tkn, &tkn);
  if (var_type) {
    Node *first_var = define_ident(tkn, &tkn, var_type, is_global);
    if (!once) {
      Node *now_var = first_var;
      while (consume(tkn, &tkn, ",")) {
        now_var->next_stmt = define_ident(tkn, &tkn, var_type, is_global);
        now_var = now_var->next_stmt;
      }
    }
    if (end_tkn != NULL) {
      *end_tkn = tkn;
    }
    return first_var;
  }
  Node *ret = assign(tkn, &tkn);
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// define_ident = "*"* ident ("[" num "]")*
static Node *define_ident(Token *tkn, Token **end_tkn, Type *define_type, bool is_global) {
  if (!define_type) {
    errorf(ER_INTERNAL, "Internal Error at define variable");
  }
  Type *now_type = calloc(sizeof(Type), 1);
  memcpy(now_type, define_type, sizeof(Type));
  int ptr_cnt = 0;
  while (consume(tkn, &tkn, "*")) {
    ++ptr_cnt;
  }

  char *ident = get_ident(tkn);
  if (ident == NULL) {
    errorf_tkn(ER_COMPILE, tkn,
              "Variable definition must be identifier.");
  }

  if (check_already_define(ident, is_global)) {
    errorf_tkn(ER_COMPILE, tkn, "This variable is already definition.");
  }
  Obj *var = new_obj(now_type, ident);
  tkn = tkn->next;

  // Size needs to be viewed from the end.
  typedef struct ArraySize ArraySize;

  struct ArraySize {
    int array_size;
    ArraySize *before;
  };

  ArraySize *top = NULL;

  while (consume(tkn, &tkn, "[")) {
    if (tkn->kind != TK_NUM_INT) {
      errorf_tkn(ER_COMPILE, tkn, "Specify the size of array.");
    }
    int array_size = tkn->val;
    tkn = tkn->next;
    if (!consume(tkn, &tkn, "]")) {
      errorf_tkn(ER_COMPILE, tkn, "Must end [");
    }

    ArraySize *now = calloc(sizeof(ArraySize), 1);
    now->array_size = array_size;
    now->before = top;
    top = now;
  }

  while (top) {
    var->type = array_to(var->type, top->array_size);
    top = top->before;
  }

  for (int i = 0; i < ptr_cnt; ++i) {
    var->type = pointer_to(var->type);
  }
  Node *ret = new_var(var);
  ret->is_var_define_only = true;
  if (is_global) {
    add_gvar(var);
  } else {
    add_lvar(var);
  }

  if (consume(tkn, &tkn, "=")) {
    ret = new_assign(ND_ASSIGN, ret, assign(tkn, &tkn));
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

// cast = ("(" get_type ")") cast |
//        unary
static Node *cast(Token *tkn, Token **end_tkn) {
  if (equal(tkn, "(")) {
    if (get_type(tkn->next, NULL) == NULL) {
      Node *ret = unary(tkn, &tkn);
      if (end_tkn != NULL) *end_tkn = tkn;
      return ret;
    }
    Type *type = get_type(tkn->next, &tkn);
    if (!consume(tkn, &tkn, ")")) {
      errorf_tkn(ER_COMPILE, tkn, "Cast must end with \")\".");
    }
    Node *ret = new_cast(cast(tkn, &tkn), type);
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
