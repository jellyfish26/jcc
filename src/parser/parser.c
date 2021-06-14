#include "parser/parser.h"

#include <stdlib.h>

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *ret = calloc(1, sizeof(Node));
  ret->kind = kind;
  ret->lhs = lhs;
  ret->rhs = rhs;
  return ret;
}

Node *new_node_int(int val) {
  Node *ret = calloc(1, sizeof(Node));
  ret->kind = ND_INT;
  ret->val = val;
  return ret;
}

// kind is ND_ASSIGN if operation only assign
Node *new_assign_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *ret = new_node(ND_ASSIGN, lhs, rhs);
  ret->assign_type = kind;
  return ret;
}

//
// About generate type
//
Type *gen_type() {
  Type *ret = calloc(1, sizeof(Type));

  Token *tkn = consume(TK_KEYWORD, "int");
  if (tkn) {
    ret->kind = TY_INT;
    ret->type_size = 4;
    ret->move_size = 1;
    return ret;
  }

  tkn = consume(TK_KEYWORD, "long");
  if (tkn) {
    while (consume(TK_KEYWORD, "long"));    // "long long ..."
    consume(TK_KEYWORD, "int");             // "long long int" or "long int"

    ret->kind = TY_LONG;
    ret->type_size = 8;
    ret->move_size = 1;
    return ret;
  }
  return NULL;
}

Type *connect_ptr_type(Type *before) {
  Type *ret = calloc(sizeof(Type), 1);

  ret->content = before;
  ret->kind = TY_PTR;
  ret->move_size = before->type_size;
  ret->type_size = 8;
  return ret;
}

Type *connect_array_type(Type *before, int array_size) {
  Type *ret = connect_ptr_type(before);

  ret->content = before;
  ret->kind = TY_ARRAY;
  ret->move_size = before->type_size;
  ret->type_size = array_size * before->type_size;
  return ret;
}

Type *get_type_for_node(Node *target) {
  if (!target->var) {
    errorf(ER_INTERNAL, "The Node(%x) do not have variable", target);
  }
  return target->var->var_type;
}

void raise_type_for_node(Node *target) {
  if (target->lhs->var &&
        (get_type_for_node(target->lhs)->kind == TY_PTR ||
         get_type_for_node(target->lhs)->kind == TY_ARRAY)) {
      target->var = target->lhs->var;
    }

    if (target->rhs->var &&
        (get_type_for_node(target->rhs)->kind == TY_PTR ||
         get_type_for_node(target->rhs)->kind == TY_ARRAY)) {
      target->var = target->rhs->var;
    }
}

//
// About variable
//

Var *add_var(Type *var_type, char *str, int len) {
  Var *ret = calloc(1, sizeof(Var));
  ret->var_type = var_type;
  ret->str = str;
  ret->len = len;

  ret->next = exp_func->vars;
  exp_func->vars = ret;
  return ret;
}

Var *find_var(Token *target) {
  char *str = calloc(target->str_len + 1, sizeof(char));
  memcpy(str, target->str, target->str_len);
  Var *ret = NULL;
  for (Var *now = exp_func->vars; now; now = now->next) {
    if (now->len != target->str_len) {
      continue;
    }

    if (memcmp(str, now->str, target->str_len) == 0) {
      ret = now;
      break;
    }
  }
  return ret;
}

void init_offset(Function *target) {
  int now_address = 0;
  Var *now_var = target->vars;
  while (now_var) {
    now_address += now_var->var_type->type_size;
    now_var->offset = now_address;

    now_var = now_var->next;
  }

  // set 16 times
  now_address += 16 - (now_address % 16);

  target->vars_size = now_address;
}

Var *down_type_level(Var *target) {
  if (!target) {
    return NULL;
  }
  Var *ret = calloc(sizeof(Var), 1);
  memcpy(ret, target, sizeof(Var));
  ret->var_type = ret->var_type->content;
  return ret;
}

// Prototype
void program();
Node *statement();
Node *assign();
Node *ternary();
Node *logical_or();
Node *logical_and();
Node *bitwise_or();
Node *bitwise_xor();
Node *bitwise_and();
Node *same_comp();
Node *size_comp();
Node *bitwise_shift();
Node *add();
Node *mul();
Node *unary();
Node *define_var();
Node *address_op();
Node *indirection();
Node *increment_and_decrement();
Node *priority();
Node *num();

void function();

Function *top_func;
Function *exp_func;

void program() {
  int i = 0;
  while (!is_eof()) {
    if (exp_func) {
      exp_func->next = calloc(1, sizeof(Function));
      exp_func = exp_func->next;
      function(exp_func);
    } else {
      exp_func = calloc(1, sizeof(Function));
      top_func = exp_func;
      function(exp_func);
    }
  }
}

// function = base_type ident "(" params?")" statement
// params = base_type ident ("," base_type ident)*
// base_type is gen_type()
void function(Function *target) {
  Type *ret_type = gen_type();

  if (ret_type) {
    target->ret_type = ret_type;
  } else {
    errorf_at(ER_COMPILE, source_token, "Undefined type.");
  }

  Token *function_ident = consume(TK_IDENT, NULL);
  if (function_ident) {
    if (!consume(TK_PUNCT, "(")) {
      errorf_at(ER_COMPILE, source_token, "Define function must start with \"(\".");
    }

    bool is_variable_defined = true;
    if (consume(TK_PUNCT, ")")) {
      is_variable_defined = false;
    }

    // Set arguments
    while (is_variable_defined) {
      Type *arg_type = gen_type();
      if (!arg_type) {
        errorf_at(ER_COMPILE, source_token, "Undefined type.");
      }

      Token *var_tkn = consume(TK_IDENT, NULL);
      if (var_tkn) {
        Var *local_var = find_var(var_tkn);
        if (local_var) {
          errorf_at(ER_COMPILE, before_token, "This variable already definition.");
        }
        local_var = add_var(arg_type, var_tkn->str, var_tkn->str_len);
        target->func_args = new_node(ND_VAR, target->func_args, NULL);
        target->func_args->var = local_var;
        target->func_argc++;
      } else {
        errorf_at(ER_COMPILE, source_token, "Must declare variable.");
      }

      if (consume(TK_PUNCT, ",")) {
        continue;
      }

      if (consume(TK_PUNCT, ")")) {
        break;
      } else {
        errorf_at(ER_COMPILE, source_token, "Define function must end with \")\".");
      }
    }
    target->func_name = function_ident->str;
    target->func_name_len = function_ident->str_len;
    target->stmt = statement();
  } else {
    errorf_at(ER_COMPILE, source_token,
              "Start the function with an identifier.");
  }
}

Node *inside_roop; // inside for or while

// statement = { statement* } |
//             ("return")? assign ";" |
//             "if" "(" assign ")" statement ("else" statement)? |
//             "for" "("define_type?; assign?; assign?")" statement |
//             "while" "(" assign ")" statement |
//             "break;" |
//             "continue;" |
//             define_type ";"
Node *statement() {
  Node *ret;

  // Block statement
  if (consume(TK_PUNCT, "{")) {
    ret = new_node(ND_BLOCK, NULL, NULL);
    Node *now = NULL;
    while (!consume(TK_PUNCT, "}")) {
      if (now) {
        now->next_stmt = statement();
        now = now->next_stmt;
      } else {
        now = statement();
        ret->next_stmt = now;
      }
    }
    return ret;
  }

  if (consume(TK_KEYWORD, "if")) {
    ret = new_node(ND_IF, NULL, NULL);

    if (!consume(TK_PUNCT, "(")) {
      errorf_at(ER_COMPILE, source_token, "If statement must start with \"(\"");
    }
    ret->judge = assign();
    if (!consume(TK_PUNCT, ")")) {
      errorf_at(ER_COMPILE, source_token, "If statement must end with \")\".");
    }
    ret->exec_if = statement();
    if (consume(TK_KEYWORD, "else")) {
      ret->exec_else = statement();
    }
    return ret;
  }

  if (consume(TK_KEYWORD, "for")) {
    Node *roop_state = inside_roop;
    ret = new_node(ND_FOR, NULL, NULL);
    inside_roop = ret;

    if (!consume(TK_PUNCT, "(")) {
      errorf_at(ER_COMPILE, source_token, "For statement must start with \"(\".");
    }

    if (!consume(TK_PUNCT, ";")) {
      ret->init_for = define_var();
      if (!consume(TK_PUNCT, ";")) {
        errorf_at(ER_COMPILE, source_token, "After defining an expression, it must end with \";\". ");
      }
    }

    if (!consume(TK_PUNCT, ";")) {
      ret->judge = assign();
      if (!consume(TK_PUNCT, ";")) {
        errorf_at(ER_COMPILE, source_token, "After defining an expression, it must end with \";\". ");
      }
    }

    if (!consume(TK_PUNCT, ")")) {
      ret->repeat_for = assign();
      if (!consume(TK_PUNCT, ")")) {
        errorf_at(ER_COMPILE, source_token, "For statement must end with \")\".");
      }
    }
    ret->stmt_for = statement();

    inside_roop = roop_state;
    return ret;
  }

  if (consume(TK_KEYWORD, "while")) {
    Node *roop_state = inside_roop;
    ret = new_node(ND_WHILE, NULL, NULL);
    inside_roop = ret;

    if (!consume(TK_PUNCT, "(")) {
      errorf_at(ER_COMPILE, source_token, "While statement must start with \"(\".");
    }
    ret->judge = assign();
    if (!consume(TK_PUNCT, ")")) {
      errorf_at(ER_COMPILE, source_token, "While statement must end with \")\".");
    }
    ret->stmt_for = statement();

    inside_roop = roop_state;
    return ret;
  }

  if (consume(TK_KEYWORD, "return")) {
    ret = new_node(ND_RETURN, assign(), NULL);

    if (!consume(TK_PUNCT, ";")) {
      errorf_at(ER_COMPILE, source_token, "Expression must end with \";\".");
    }
    return ret;
  }

  if (consume(TK_KEYWORD, "break")) {
    if (!inside_roop) {
      errorf_at(ER_COMPILE, before_token, "%s",
                "Not whithin loop");
    }
    ret = new_node(ND_LOOPBREAK, NULL, NULL);
    ret->lhs = inside_roop;
    if (!consume(TK_PUNCT, ";")) {
      errorf_at(ER_COMPILE, source_token, "Expression must end with \";\".");
    }
    return ret;
  }

  if (consume(TK_KEYWORD, "continue")) {
    if (!inside_roop) {
      errorf_at(ER_COMPILE, before_token, "%s",
                "continue statement not within loop");
    }
    ret = new_node(ND_CONTINUE, NULL, NULL);
    ret->lhs = inside_roop;
    if (!consume(TK_PUNCT, ";")) {
      errorf_at(ER_COMPILE, source_token, "Expression must end with \";\".");
    }
    return ret;
  }

  ret = define_var();
  if (!consume(TK_PUNCT, ";")) {
    errorf_at(ER_COMPILE, source_token, "Expression must end with \";\".");
  }

  return ret;
}

// define_var = base_type "*"* ident ("[" num "]")* ("=" assign)? |
//              assign
Node *define_var() {
  Type *var_type = gen_type();

  // define variable
  if (var_type) {
    int ptr_cnt = 0;
    while (consume(TK_PUNCT, "*")) {
      ++ptr_cnt;
    }

    Token *tkn = consume(TK_IDENT, NULL);
    Node *ret = new_node(ND_VAR, NULL, NULL);

    if (!tkn) {
      errorf_at(ER_COMPILE, source_token,
                "Variable definition must be identifier.");
    }

    Var *result = find_var(tkn);
    if (result) {
      errorf_at(ER_COMPILE, before_token,
                "This variable is already definition.");
    }
    ret->var = add_var(var_type, tkn->str, tkn->str_len);

    // Size needs to be viewed from the end.
    typedef struct ArraySize ArraySize;

    struct ArraySize {
      int array_size;
      ArraySize *before;
    };

    ArraySize *top = NULL;

    while (consume(TK_PUNCT, "[")) {
      Token *array_size = consume(TK_NUM_INT, NULL);
      if (!array_size) {
        errorf_at(ER_COMPILE, source_token, "Specify the size of array.");
      }
      if (!consume(TK_PUNCT, "]")) {
        errorf_at(ER_COMPILE, source_token, "Must end [");
      }

      ArraySize *now = calloc(sizeof(ArraySize), 1);
      now->array_size = array_size->val;
      now->before = top;
      top = now;
    }

    while (top) {
      ret->var->var_type = connect_array_type(ret->var->var_type, top->array_size);
      top = top->before;
    }

    for (int i = 0; i < ptr_cnt; ++i) {
      ret->var->var_type = connect_ptr_type(ret->var->var_type);
    }

    if (consume(TK_PUNCT, "=")) {
      ret = new_assign_node(ND_ASSIGN, ret, assign());
    }
    return ret;
  }
  return assign();
}

// assign = ternary ("=" assign | "+=" assign | "-=" assign
//                   "*=" assign | "/=" assign | "%=" assign
//                   "<<=" assign | ">>=" assign
//                   "&=" assign | "^=" assign | "|=" assign)?
Node *assign() {
  Node *ret = ternary();

  if (consume(TK_PUNCT, "=")) {
    ret = new_assign_node(ND_ASSIGN, ret, assign());
  } else if (consume(TK_PUNCT, "+=")) {
    ret = new_assign_node(ND_ADD, ret, assign());
  } else if (consume(TK_PUNCT, "-=")) {
    ret = new_assign_node(ND_SUB, ret, assign());
  } else if (consume(TK_PUNCT, "*=")) {
    ret = new_assign_node(ND_MUL, ret, assign());
  } else if (consume(TK_PUNCT, "/=")) {
    ret = new_assign_node(ND_DIV, ret, assign());
  } else if (consume(TK_PUNCT, "%=")) {
    ret = new_assign_node(ND_REMAINDER, ret, assign());
  } else if (consume(TK_PUNCT, "<<=")) {
    ret = new_assign_node(ND_LEFTSHIFT, ret, assign());
  } else if (consume(TK_PUNCT, ">>=")) {
    ret = new_assign_node(ND_RIGHTSHIFT, ret, assign());
  } else if (consume(TK_PUNCT, "&=")) {
    ret = new_assign_node(ND_BITWISEAND, ret, assign());
  } else if (consume(TK_PUNCT, "^=")) {
    ret = new_assign_node(ND_BITWISEXOR, ret, assign());
  } else if (consume(TK_PUNCT, "|=")) {
    ret = new_assign_node(ND_BITWISEOR, ret, assign());
  }
  return ret;
}

// ternary = logical_or ("?" ternary ":" ternary)?
Node *ternary() {
  Node *ret = logical_or();

  if (consume(TK_PUNCT, "?")) {
    Node *tmp = new_node(ND_TERNARY, NULL, NULL);
    tmp->lhs = ternary();
    if (!consume(TK_PUNCT, ":")) {
      errorf_at(ER_COMPILE, source_token, "The ternary operator requires \":\".");
    }
    tmp->rhs = ternary();
    tmp->exec_if = ret;
    ret = tmp;
  }
  return ret;
}

// logical_or = logical_and ("||" logical_or)?
Node *logical_or() {
  Node *ret = logical_and();

  if (consume(TK_PUNCT, "||")) {
    ret = new_node(ND_LOGICALOR, ret, logical_or());
  }
  return ret;
}

// logical_and = bitwise_or ("&&" logical_and)?
Node *logical_and() {
  Node *ret = bitwise_or();

  if (consume(TK_PUNCT, "&&")) {
    ret = new_node(ND_LOGICALAND, ret, logical_and());
  }
  return ret;
}

// bitwise_or = bitwise_xor ("|" bitwise_or)?
Node *bitwise_or() {
  Node *ret = bitwise_xor();
  if (consume(TK_PUNCT, "|")) {
    ret = new_node(ND_BITWISEOR, ret, bitwise_or());
  }
  return ret;
}

// bitwise_xor = bitwise_and ("^" bitwise_xor)?
Node *bitwise_xor() {
  Node *ret = bitwise_and();
  if (consume(TK_PUNCT, "^")) {
    ret = new_node(ND_BITWISEXOR, ret, bitwise_xor());
  }
  return ret;
}

// bitwise_and = same_comp ("&" bitwise_and)?
Node *bitwise_and() {
  Node *ret = same_comp();
  if (consume(TK_PUNCT, "&")) {
    ret = new_node(ND_BITWISEAND, ret, bitwise_and());
  }
  return ret;
}

// same_comp = size_comp ("==" same_comp | "!=" sama_comp)?
Node *same_comp() {
  Node *ret = size_comp();
  if (consume(TK_PUNCT, "==")) {
    ret = new_node(ND_EQ, ret, same_comp());
  } else if (consume(TK_PUNCT, "!=")) {
    ret = new_node(ND_NEQ, ret, same_comp());
  }
  return ret;
}

// size_comp = bitwise_shift ("<" size_comp  | ">" size_comp | "<=" size_comp | ">=" size_comp)?
Node *size_comp() {
  Node *ret = bitwise_shift();
  if (consume(TK_PUNCT, "<")) {
    ret = new_node(ND_LC, ret, size_comp());
  } else if (consume(TK_PUNCT, ">")) {
    ret = new_node(ND_RC, ret, size_comp());
  } else if (consume(TK_PUNCT, "<=")) {
    ret = new_node(ND_LEC, ret, size_comp());
  } else if (consume(TK_PUNCT, ">=")) {
    ret = new_node(ND_REC, ret, size_comp());
  }
  return ret;
}

// bitwise_shift = add ("<<" bitwise_shift | ">>" bitwise_shift)?
Node *bitwise_shift() {
  Node *ret = add();
  if (consume(TK_PUNCT, "<<")) {
    ret = new_node(ND_LEFTSHIFT, ret, bitwise_shift());
  } else if (consume(TK_PUNCT, ">>")) {
    ret = new_node(ND_RIGHTSHIFT, ret, bitwise_shift());
  }
  return ret;
}


// add = mul ("+" add | "-" add)?
Node *add() {
  Node *ret = mul();
  if (consume(TK_PUNCT, "+")) {
    ret = new_node(ND_ADD, ret, add());
    raise_type_for_node(ret);
  } else if (consume(TK_PUNCT, "-")) {
    ret = new_node(ND_SUB, ret, add());
    raise_type_for_node(ret);
  }
  return ret;
}

// mul = unary ("*" mul | "/" mul | "%" mul)?
Node *mul() {
  Node *ret = unary();
  if (consume(TK_PUNCT, "*")) {
    ret = new_node(ND_MUL, ret, mul());
  } else if (consume(TK_PUNCT, "/")) {
    ret = new_node(ND_DIV, ret, mul());
  } else if (consume(TK_PUNCT, "%")) {
    ret = new_node(ND_REMAINDER, ret, mul());
  }
  return ret;
}

// unary = address_op |
//         ("+" | "-" | "!" | "~") unary
Node *unary() {
  if (consume(TK_PUNCT, "+")) {
    Node *ret = new_node(ND_ADD, new_node_int(0), unary());
    raise_type_for_node(ret);
    return ret;
  } else if (consume(TK_PUNCT, "-")) {
    Node *ret = new_node(ND_SUB, new_node_int(0), unary());
    raise_type_for_node(ret);
    return ret;
  } else if (consume(TK_PUNCT, "!")) {
    return new_node(ND_LOGICALNOT, unary(), NULL);
  } else if (consume(TK_PUNCT, "~")) {
    return new_node(ND_BITWISENOT, unary(), NULL);
  }
  return address_op();
}

// address_op = "&"? indirection
Node *address_op() {
  if (consume(TK_PUNCT, "&")) {
    Node *ret = new_node(ND_ADDR, indirection(), NULL);
    return ret;
  }
  return indirection();
}

// indirection = (increment_and_decrement | "*" indirection)
Node *indirection() {
  if (consume(TK_PUNCT, "*")) {
    Node *ret = new_node(ND_CONTENT, indirection(), NULL);
    ret->var = down_type_level(ret->lhs->var);
    return ret;
  }
  return increment_and_decrement();
}

// increment_and_decrement = priority |
//                           ("++" | "--") priority |
//                           priority ("++" | "--")
Node *increment_and_decrement() {
  if (consume(TK_PUNCT, "++")) {
    return new_node(ND_PREFIX_INC, priority(), NULL);
  } else if (consume(TK_PUNCT, "--")) {
    return new_node(ND_PREFIX_DEC, priority(), NULL);
  }

  Node *ret = priority();
  if (consume(TK_PUNCT, "++")) {
    return new_node(ND_SUFFIX_INC, ret, NULL);
  } else if (consume(TK_PUNCT, "--")) {
    return new_node(ND_SUFFIX_DEC, ret, NULL);
  }
  return ret;
}

// priority = num |
//            "(" assign ")" |
//            ident ("(" params? ")")? |
//            ident ("[" assign "]")* |
// params = assign ("," assign)?
// base_type is gen_type()
Node *priority() {
  if (consume(TK_PUNCT, "(")) {
    Node *ret = assign();
    
    if (!consume(TK_PUNCT, ")")) {
      errorf_at(ER_COMPILE, source_token, "\"(\" and \")\" should be written in pairs.");
    }
    return ret;
  }

  Token *tkn = consume(TK_IDENT, NULL);

  // function call
  if (tkn) {
    if (consume(TK_PUNCT, "(")) {
      Node *ret = new_node(ND_FUNCCALL, NULL, NULL);
      ret->func_name = tkn->str;
      ret->func_name_len = tkn->str_len;

      if (consume(TK_PUNCT, ")")) {
        return ret;
      }

      int argc = 0;
      Node *now_arg = NULL;
      while (true) {
        Node *tmp = assign();
        tmp->func_arg = now_arg;
        now_arg = tmp;

        if (consume(TK_PUNCT, ",")) {
          continue;
        }

        if (!consume(TK_PUNCT, ")")) {
          errorf_at(ER_COMPILE, source_token, "Function call must end with \")\".");
        }
        break;
      }
      ret->func_arg = now_arg;
      return ret;
    }
  }

  // used variable
  if (tkn) {
    Var *target = find_var(tkn);
    if (!target) {
      errorf_at(ER_COMPILE, before_token, "This variable is not definition.");
    }
    Node *ret = new_node(ND_VAR, NULL, NULL);
    ret->var = target;
    while (consume(TK_PUNCT, "[")) {
      ret = new_node(ND_ADD, ret, assign());
      ret->var = down_type_level(ret->lhs->var);

      ret = new_node(ND_CONTENT, ret, NULL);
      ret->var = ret->lhs->var;
      if (!consume(TK_PUNCT, "]")) {
        errorf_at(ER_COMPILE, source_token, "Must use \"[\".");
      }
    }
    return ret;
  }

  return num();
}

Node *num() {
  Token *tkn = consume(TK_NUM_INT, NULL);
  if (!tkn) {
    errorf_at(ER_COMPILE, source_token, "Not value.");
  }
  return new_node_int(tkn->val);
}
