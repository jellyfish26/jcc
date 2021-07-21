#include "parser/parser.h"
#include "token/tokenize.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type *compare_type(Type *left, Type *right) {
  if (left == NULL || right == NULL) {
    if (left < right) {
      return right;
    }
    return left;
  }
  if (left->kind < right->kind) {
    return right;
  }
  return left;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *ret = calloc(1, sizeof(Node));
  ret->kind = kind;
  ret->lhs = lhs;
  ret->rhs = rhs;
  if (lhs != NULL && rhs != NULL) {
    ret->equation_type = compare_type(lhs->equation_type, rhs->equation_type);
  } else if (lhs != NULL) {
    ret->equation_type = lhs->equation_type;
  } else if (rhs != NULL) {
    ret->equation_type = rhs->equation_type;
  }
  return ret;
}

Node *new_node_num(int val) {
  Node *ret = calloc(1, sizeof(Node));
  ret->kind = ND_INT;
  ret->val = val;
  ret->equation_type = new_type(TY_INT, false);
  return ret;
}

// kind is ND_ASSIGN if operation only assign
Node *new_assign_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *ret = new_node(ND_ASSIGN, lhs, rhs);
  ret->assign_type = kind;
  return ret;
}

static Type *get_type(Token *tkn, Token **end_tkn) {
  if (consume(tkn, &tkn, TK_KEYWORD, "char")) {
    *end_tkn = tkn;
    return new_type(TY_CHAR, true);
  }
  if (consume(tkn, &tkn, TK_KEYWORD, "short")) {
    *end_tkn = tkn;
    return new_type(TY_SHORT, true);
  }
  if (consume(tkn, &tkn, TK_KEYWORD, "int")) {
    *end_tkn = tkn;
    return new_type(TY_INT, true);
  }
  if (consume(tkn, &tkn, TK_KEYWORD, "long")) {
    while (consume(tkn, &tkn, TK_KEYWORD, "long"));
    consume(tkn, &tkn, TK_KEYWORD, "int");
    *end_tkn = tkn;
    return new_type(TY_LONG, true);
  }
  return NULL;
}

void link_var_to_node(Node *target, Obj *var) {
  target->use_var = var;
  target->equation_type = var->type;
}

Node *last_stmt(Node *now) {
  while (now->next_stmt) {
    now = now->next_stmt;
  }
  return now;
}

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
static Node *unary(Token *tkn, Token **end_tkn);
static Node *address_op(Token *tkn, Token **end_tkn);
static Node *indirection(Token *tkn, Token **end_tkn);
static Node *increment_and_decrement(Token *tkn, Token **end_tkn);
static Node *priority(Token *tkn, Token **end_tkn);
static Node *num(Token *tkn, Token **end_tkn);

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
  Token *global_ident = tkn;
  if (consume(tkn, &tkn, TK_IDENT, NULL)) {
    if (!consume(tkn, &tkn, TK_PUNCT, "(")) {
      define_var(type_tkn, &tkn, false, true);
      consume(tkn, &tkn, TK_PUNCT, ";");
      if (end_tkn != NULL) *end_tkn = tkn;
      return false;
    }

    // Function define
    new_scope_definition();
    bool is_var_defined = true;
    if (consume(tkn, &tkn, TK_PUNCT, ")")) {
      is_var_defined = false;
    }

    // Set arguments
    while (is_var_defined) {
      Type *arg_type = get_type(tkn, &tkn);
      if (!arg_type) {
        errorf_tkn(ER_COMPILE, tkn, "Undefined type.");
      }

      // Variable define in function arguments
      if (consume(tkn, NULL, TK_IDENT, NULL)) {
        Token *ident_tkn = tkn;
        tkn = tkn->next;
        if (find_var(ident_tkn->str, ident_tkn->str_len) != NULL) {
          errorf_tkn(ER_COMPILE, ident_tkn, "This variable already definition.");
        }
        Obj *lvar = new_obj(arg_type, ident_tkn->str, ident_tkn->str_len);
        add_lvar(lvar);
        func->func_args = new_node(ND_VAR, func->func_args, NULL);
        func->func_args->is_var_define_only = true;
        link_var_to_node(func->func_args, lvar);
        func->func_argc++;
      } else {
        errorf_tkn(ER_COMPILE, tkn, "Must declare variable.");
      }

      if (consume(tkn, &tkn, TK_PUNCT, ",")) {
        continue;
      }

      if (consume(tkn, &tkn, TK_PUNCT, ")")) {
        break;
      } else {
        errorf_tkn(ER_COMPILE, tkn, "Define function must tend with \")\".");
      }
    }
    func->func_name = global_ident->str;
    func->func_name_len = global_ident->str_len;
    func->stmt = statement(tkn, &tkn, false);
    out_scope_definition();
    func->vars_size = init_offset();
  } else {
    errorf_tkn(ER_COMPILE, tkn, "Start the function with an identifier.");
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return true;
}

Node *inside_roop; // inside for or while

// statement = { statement* } |
//             ("return")? assign ";" |
//             "if" "(" define_var(true, NULL) ")" statement ("else" statement)?
//             | "for" "(" define_var(true, NULL)? ";" assign? ";" assign?")"
//             statement | "while" "(" define_var(true, NULL) ")" statement |
//             "break;" |
//             "continue;" |
//             define_var(false) ";"
Node *statement(Token *tkn, Token **end_tkn, bool new_scope) {
  Node *ret;

  // Block statement
  if (consume(tkn, &tkn, TK_PUNCT, "{")) {
    if (new_scope) {
      new_scope_definition();
    }
    ret = new_node(ND_BLOCK, NULL, NULL);
    Node *now = NULL;
    while (!consume(tkn, &tkn, TK_PUNCT, "}")) {
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

  if (consume(tkn, &tkn, TK_KEYWORD, "if")) {
    new_scope_definition();
    ret = new_node(ND_IF, NULL, NULL);

    if (!consume(tkn, &tkn, TK_PUNCT, "(")) {
      errorf_tkn(ER_COMPILE, tkn, "If statement must start with \"(\"");
    }
    ret->judge = define_var(tkn, &tkn, true, false);
    Obj *top_var = lvars->objs;
    if (!consume(tkn, &tkn, TK_PUNCT, ")")) {
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
    if (consume(tkn, &tkn, TK_KEYWORD, "else")) {
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

  if (consume(tkn, &tkn, TK_KEYWORD, "for")) {
    new_scope_definition();
    Node *roop_state = inside_roop;
    ret = new_node(ND_FOR, NULL, NULL);
    inside_roop = ret;

    if (!consume(tkn, &tkn, TK_PUNCT, "(")) {
      errorf_tkn(ER_COMPILE, tkn, "For statement must start with \"(\".");
    }

    if (!consume(tkn, &tkn, TK_PUNCT, ";")) {
      ret->init_for = define_var(tkn, &tkn, true, false);
      if (!consume(tkn, &tkn, TK_PUNCT, ";")) {
        errorf_tkn(ER_COMPILE, tkn, "After defining an expression, it must end with \";\". ");
      }
    }

    if (!consume(tkn, &tkn, TK_PUNCT, ";")) {
      ret->judge = assign(tkn, &tkn);
      if (!consume(tkn, &tkn, TK_PUNCT, ";")) {
        errorf_tkn(ER_COMPILE, tkn, "After defining an expression, it must end with \";\". ");
      }
    }

    if (!consume(tkn, &tkn, TK_PUNCT, ")")) {
      ret->repeat_for = assign(tkn, &tkn);
      if (!consume(tkn, &tkn, TK_PUNCT, ")")) {
        errorf_tkn(ER_COMPILE, tkn, "For statement must end with \")\".");
      }
    }
    ret->stmt_for = statement(tkn, &tkn, false);
    out_scope_definition();

    inside_roop = roop_state;
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  if (consume(tkn, &tkn, TK_KEYWORD, "while")) {
    new_scope_definition();
    Node *roop_state = inside_roop;
    ret = new_node(ND_WHILE, NULL, NULL);
    inside_roop = ret;

    if (!consume(tkn, &tkn, TK_PUNCT, "(")) {
      errorf_tkn(ER_COMPILE, tkn,
                "While statement must start with \"(\".");
    }
    ret->judge = assign(tkn, &tkn);
    if (!consume(tkn, &tkn, TK_PUNCT, ")")) {
      errorf_tkn(ER_COMPILE, tkn,
                "While statement must end with \")\".");
    }
    ret->stmt_for = statement(tkn, &tkn, false);

    inside_roop = roop_state;
    out_scope_definition();
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  if (consume(tkn, &tkn, TK_KEYWORD, "return")) {
    ret = new_node(ND_RETURN, assign(tkn, &tkn), NULL);

    if (!consume(tkn, &tkn, TK_PUNCT, ";")) {
      errorf_tkn(ER_COMPILE, tkn, "Expression must end with \";\".");
    }
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  if (consume(tkn, &tkn, TK_KEYWORD, "break")) {
    if (!inside_roop) {
      errorf_tkn(ER_COMPILE, tkn, "%s", "Not whithin loop");
    }
    ret = new_node(ND_LOOPBREAK, NULL, NULL);
    ret->lhs = inside_roop;
    if (!consume(tkn, &tkn, TK_PUNCT, ";")) {
      errorf_tkn(ER_COMPILE, tkn, "Expression must end with \";\".");
    }
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  if (consume(tkn, &tkn, TK_KEYWORD, "continue")) {
    if (!inside_roop) {
      errorf_tkn(ER_COMPILE, tkn, "%s", "continue statement not within loop");
    }
    ret = new_node(ND_CONTINUE, NULL, NULL);
    ret->lhs = inside_roop;
    if (!consume(tkn, &tkn, TK_PUNCT, ";")) {
      errorf_tkn(ER_COMPILE, tkn, "Expression must end with \";\".");
    }
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  ret = define_var(tkn, &tkn, false, false);
  if (!consume(tkn, &tkn, TK_PUNCT, ";")) {
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
      while (consume(tkn, &tkn, TK_PUNCT, ",")) {
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
  while (consume(tkn, &tkn, TK_PUNCT, "*")) {
    ++ptr_cnt;
  }

  if (!consume(tkn, NULL, TK_IDENT, NULL)) {
    errorf_tkn(ER_COMPILE, tkn,
              "Variable definition must be identifier.");
  }

  if (check_already_define(tkn->str, tkn->str_len, is_global)) {
    errorf_tkn(ER_COMPILE, tkn, "This variable is already definition.");
  }
  Obj *var = new_obj(now_type, tkn->str, tkn->str_len);
  consume(tkn, &tkn, TK_IDENT, NULL);

  // Size needs to be viewed from the end.
  typedef struct ArraySize ArraySize;

  struct ArraySize {
    int array_size;
    ArraySize *before;
  };

  ArraySize *top = NULL;

  while (consume(tkn, &tkn, TK_PUNCT, "[")) {
    if (!consume(tkn, NULL, TK_NUM_INT, NULL)) {
      errorf_tkn(ER_COMPILE, tkn, "Specify the size of array.");
    }
    int array_size = tkn->val;
    consume(tkn, &tkn, TK_NUM_INT, NULL);
    if (!consume(tkn, &tkn, TK_PUNCT, "]")) {
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
  Node *ret = new_node(ND_VAR, NULL, NULL);
  ret->is_var_define_only = true;
  link_var_to_node(ret, var);
  if (is_global) {
    add_gvar(var);
  } else {
    add_lvar(var);
  }

  if (consume(tkn, &tkn, TK_PUNCT, "=")) {
    ret = new_assign_node(ND_ASSIGN, ret, assign(tkn, &tkn));
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

  if (consume(tkn, &tkn, TK_PUNCT, "=")) {
    ret = new_assign_node(ND_ASSIGN, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "+=")) {
    ret = new_assign_node(ND_ADD, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "-=")) {
    ret = new_assign_node(ND_SUB, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "*=")) {
    ret = new_assign_node(ND_MUL, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "/=")) {
    ret = new_assign_node(ND_DIV, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "%=")) {
    ret = new_assign_node(ND_REMAINDER, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "<<=")) {
    ret = new_assign_node(ND_LEFTSHIFT, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, ">>=")) {
    ret = new_assign_node(ND_RIGHTSHIFT, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "&=")) {
    ret = new_assign_node(ND_BITWISEAND, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "^=")) {
    ret = new_assign_node(ND_BITWISEXOR, ret, assign(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "|=")) {
    ret = new_assign_node(ND_BITWISEOR, ret, assign(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// ternary = logical_or ("?" ternary ":" ternary)?
static Node *ternary(Token *tkn, Token **end_tkn) {
  Node *ret = logical_or(tkn, &tkn);

  if (consume(tkn, &tkn, TK_PUNCT, "?")) {
    Node *tmp = new_node(ND_TERNARY, NULL, NULL);
    tmp->lhs = ternary(tkn, &tkn);
    if (!consume(tkn, &tkn, TK_PUNCT, ":")) {
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

  if (consume(tkn, &tkn, TK_PUNCT, "||")) {
    ret = new_node(ND_LOGICALOR, ret, logical_or(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// logical_and = bitwise_or ("&&" logical_and)?
static Node *logical_and(Token *tkn, Token **end_tkn) {
  Node *ret = bitwise_or(tkn, &tkn);

  if (consume(tkn, &tkn, TK_PUNCT, "&&")) {
    ret = new_node(ND_LOGICALAND, ret, logical_and(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// bitwise_or = bitwise_xor ("|" bitwise_or)?
static Node *bitwise_or(Token *tkn, Token **end_tkn) {
  Node *ret = bitwise_xor(tkn, &tkn);
  if (consume(tkn, &tkn, TK_PUNCT, "|")) {
    ret = new_node(ND_BITWISEOR, ret, bitwise_or(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// bitwise_xor = bitwise_and ("^" bitwise_xor)?
static Node *bitwise_xor(Token *tkn, Token **end_tkn) {
  Node *ret = bitwise_and(tkn, &tkn);
  if (consume(tkn, &tkn, TK_PUNCT, "^")) {
    ret = new_node(ND_BITWISEXOR, ret, bitwise_xor(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// bitwise_and = same_comp ("&" bitwise_and)?
static Node *bitwise_and(Token *tkn, Token **end_tkn) {
  Node *ret = same_comp(tkn, &tkn);
  if (consume(tkn, &tkn, TK_PUNCT, "&")) {
    ret = new_node(ND_BITWISEAND, ret, bitwise_and(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// same_comp = size_comp ("==" same_comp | "!=" sama_comp)?
static Node *same_comp(Token *tkn, Token **end_tkn) {
  Node *ret = size_comp(tkn, &tkn);
  if (consume(tkn, &tkn, TK_PUNCT, "==")) {
    ret = new_node(ND_EQ, ret, same_comp(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "!=")) {
    ret = new_node(ND_NEQ, ret, same_comp(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// size_comp = bitwise_shift ("<" size_comp  | ">" size_comp | "<=" size_comp |
// ">=" size_comp)?
static Node *size_comp(Token *tkn, Token **end_tkn) {
  Node *ret = bitwise_shift(tkn, &tkn);
  if (consume(tkn, &tkn, TK_PUNCT, "<")) {
    ret = new_node(ND_LC, ret, size_comp(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, ">")) {
    ret = new_node(ND_RC, ret, size_comp(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "<=")) {
    ret = new_node(ND_LEC, ret, size_comp(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, ">=")) {
    ret = new_node(ND_REC, ret, size_comp(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// bitwise_shift = add ("<<" bitwise_shift | ">>" bitwise_shift)?
static Node *bitwise_shift(Token *tkn, Token **end_tkn) {
  Node *ret = add(tkn, &tkn);
  if (consume(tkn, &tkn, TK_PUNCT, "<<")) {
    ret = new_node(ND_LEFTSHIFT, ret, bitwise_shift(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, ">>")) {
    ret = new_node(ND_RIGHTSHIFT, ret, bitwise_shift(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// add = mul ("+" add | "-" add)?
static Node *add(Token *tkn, Token **end_tkn) {
  Node *ret = mul(tkn, &tkn);
  if (consume(tkn, &tkn, TK_PUNCT, "+")) {
    ret = new_node(ND_ADD, ret, add(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "-")) {
    ret = new_node(ND_SUB, ret, add(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// mul = unary ("*" mul | "/" mul | "%" mul)?
static Node *mul(Token *tkn, Token **end_tkn) {
  Node *ret = unary(tkn, &tkn);
  if (consume(tkn, &tkn, TK_PUNCT, "*")) {
    ret = new_node(ND_MUL, ret, mul(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "/")) {
    ret = new_node(ND_DIV, ret, mul(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "%")) {
    ret = new_node(ND_REMAINDER, ret, mul(tkn, &tkn));
  }
  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// unary = "sizeof" unary
//         ("+" | "-" | "!" | "~")? address_op
static Node *unary(Token *tkn, Token **end_tkn) {
  if (consume(tkn, &tkn, TK_KEYWORD, "sizeof")) {
    Node *ret = new_node(ND_SIZEOF, unary(tkn, &tkn), NULL);
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }
  Node *ret = NULL;
  if (consume(tkn, &tkn, TK_PUNCT, "+")) {
    ret = new_node(ND_ADD, new_node_num(0), address_op(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "-")) {
    ret = new_node(ND_SUB, new_node_num(0), address_op(tkn, &tkn));
  } else if (consume(tkn, &tkn, TK_PUNCT, "!")) {
    ret = new_node(ND_LOGICALNOT, address_op(tkn, &tkn), NULL);
  } else if (consume(tkn, &tkn, TK_PUNCT, "~")) {
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
  if (consume(tkn, &tkn, TK_PUNCT, "&")) {
    ret = new_node(ND_ADDR, indirection(tkn, &tkn), NULL);
    Type *addr_type = new_type(TY_ADDR, false);
    addr_type->content = ret->equation_type;
    ret->equation_type = addr_type;
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
  if (consume(tkn, &tkn, TK_PUNCT, "*")) {
    ret = new_node(ND_CONTENT, indirection(tkn, &tkn), NULL);
    if (ret->equation_type) {
      ret->equation_type = ret->equation_type->content;
    }
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
  if (consume(tkn, &tkn, TK_PUNCT, "++")) {
    ret = new_node(ND_PREFIX_INC, priority(tkn, &tkn), NULL);
  } else if (consume(tkn, &tkn, TK_PUNCT, "--")) {
    ret = new_node(ND_PREFIX_INC, priority(tkn, &tkn), NULL);
  }

  if (ret != NULL) {
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  ret = priority(tkn, &tkn);
  if (consume(tkn, &tkn, TK_PUNCT, "++")) {
    ret = new_node(ND_SUFFIX_INC, ret, NULL);
  } else if (consume(tkn, &tkn, TK_PUNCT, "--")) {
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
  if (consume(tkn, &tkn, TK_PUNCT, "(")) {
    // GNU Statements and Declarations
    if (consume(tkn, NULL, TK_PUNCT, "{")) {
      Node *ret = statement(tkn, &tkn, true);
      if (!consume(tkn, &tkn, TK_PUNCT, ")")) {
        errorf_tkn(ER_COMPILE, tkn, "\"(\" and \")\" should be written in pairs.");
      }
      if (end_tkn != NULL) *end_tkn = tkn;
      return ret;
    }
    Node *ret = assign(tkn, &tkn);

    if (!consume(tkn, &tkn, TK_PUNCT, ")")) {
      errorf_tkn(ER_COMPILE, tkn, "\"(\" and \")\" should be written in pairs.");
    }
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  // String literal
  if (consume(tkn, NULL, TK_STR, NULL)) {
    Obj *var = new_obj(new_type(TY_STR, false), tkn->str_lit, strlen(tkn->str_lit));
    tkn = tkn->next;
    Node *ret = new_node(ND_VAR, NULL, NULL);
    ret->is_var_define_only = false;
    link_var_to_node(ret, var);
    add_gvar(var);
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  if (consume(tkn, NULL, TK_IDENT, NULL)) {
    Token *ident = tkn;
    tkn = tkn->next;
    // function call
    if (consume(tkn, &tkn, TK_PUNCT, "(")) {
      Node *ret = new_node(ND_FUNCCALL, NULL, NULL);
      ret->func = new_obj(new_type(TY_INT, false), ident->str, ident->str_len); // (Warn: Temporary type)

      if (consume(tkn, &tkn, TK_PUNCT, ")")) {
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

        if (consume(tkn, &tkn, TK_PUNCT, ",")) {
          continue;
        }

        if (!consume(tkn, &tkn, TK_PUNCT, ")")) {
          errorf_tkn(ER_COMPILE, tkn, "Function call must end with \")\".");
        }
        break;
      }
      ret->func->argc = argc;
      if (end_tkn != NULL) *end_tkn = tkn;
      return ret;
    } else {
      // use variable
      Obj *use_var = find_var(ident->str, ident->str_len);
      if (use_var == NULL) {
        errorf_tkn(ER_COMPILE, ident, "This variable is not definition.");
      }
      Node *ret = new_node(ND_VAR, NULL, NULL);
      ret->is_var_define_only = false;
      link_var_to_node(ret, use_var);
      while (consume(tkn, &tkn, TK_PUNCT, "[")) {
        ret = new_node(ND_ADD, ret, assign(tkn, &tkn));

        ret = new_node(ND_CONTENT, ret, NULL);
        ret->equation_type = ret->equation_type->content;
        if (!consume(tkn, &tkn, TK_PUNCT, "]")) {
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
  if (consume(tkn, NULL, TK_NUM_INT, NULL)) {
    if (end_tkn != NULL) *end_tkn = tkn->next;
    return new_node_num(tkn->val);
  }
  if (!consume(tkn, NULL, TK_CHAR, NULL)) {
    errorf_tkn(ER_COMPILE, tkn, "Not value.");
  }
  if (end_tkn != NULL) *end_tkn = tkn->next;
  return new_node_num(tkn->c_lit);
}
