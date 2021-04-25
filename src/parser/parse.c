#include "parser.h"

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
Node *prefix_inc_or_dec();
Node *priority();
Node *num();

void *function();

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
void *function(Function *target) {
  Type *ret_type = gen_type();

  if (ret_type) {
    target->ret_type = ret_type;
  } else {
    errorf_at(ER_COMPILE, source_token, "Undefined type.");
  }

  Token *tkn = use_any_kind(TK_IDENT);
  if (tkn) {
    use_expect_symbol("(");
    if (!use_symbol(")")) {
      // Set arguments
      while (true) {
        Type *arg_type = gen_type();
        if (!arg_type) {
          errorf_at(ER_COMPILE, source_token, "Undefined type.");
        }

        Token *tkn = use_any_kind(TK_IDENT);
        if (tkn) {
          Var *local_var = find_var(tkn);
          if (local_var) {
            errorf_at(ER_COMPILE, before_token,
                      "This variable is already definition.");
          }
          local_var = add_var(arg_type, tkn->str, tkn->str_len);

          target->func_args = new_node(ND_VAR, target->func_args, NULL);
          target->func_args->var = local_var;
          target->func_argc++;
          if (use_symbol(",")) {
            continue;
          }
          use_expect_symbol(")");
          break;
        } else {
          errorf_at(ER_COMPILE, source_token, "Declare variable.");
        }
      }
    }
    target->func_name = tkn->str;
    target->func_name_len = tkn->str_len;
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
  if (use_symbol("{")) {
    ret = new_node(ND_BLOCK, NULL, NULL);
    Node *now = NULL;
    while (!use_symbol("}")) {
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

  if (use_any_kind(TK_IF)) {
    ret = new_node(ND_IF, NULL, NULL);
    use_expect_symbol("(");
    ret->judge = assign();
    use_expect_symbol(")");
    ret->exec_if = statement();
    if (use_any_kind(TK_ELSE)) {
      ret->exec_else = statement();
    }
    return ret;
  }

  if (use_any_kind(TK_FOR)) {
    Node *roop_state = inside_roop;
    ret = new_node(ND_FOR, NULL, NULL);
    inside_roop = ret;

    use_expect_symbol("(");
    if (!use_symbol(";")) {
      ret->init_for = define_var();
      use_expect_symbol(";");
    }

    if (!use_symbol(";")) {
      ret->judge = assign();
      use_expect_symbol(";");
    }

    if (!use_symbol(")")) {
      ret->repeat_for = assign();
      use_expect_symbol(")");
    }
    ret->stmt_for = statement();

    inside_roop = roop_state;
    return ret;
  }

  if (use_any_kind(TK_WHILE)) {
    Node *roop_state = inside_roop;
    ret = new_node(ND_WHILE, NULL, NULL);
    inside_roop = ret;

    use_expect_symbol("(");
    ret->judge = assign();
    use_expect_symbol(")");
    ret->stmt_for = statement();

    inside_roop = roop_state;
    return ret;
  }

  if (use_any_kind(TK_RETURN)) {
    ret = new_node(ND_RETURN, assign(), NULL);
    use_expect_symbol(";");
    return ret;
  }

  if (use_any_kind(TK_BREAK)) {
    if (!inside_roop) {
      errorf_at(ER_COMPILE, before_token, "%s",
                "break statement not whithin loop");
    }
    ret = new_node(ND_LOOPBREAK, NULL, NULL);
    ret->lhs = inside_roop;
    use_expect_symbol(";");
    return ret;
  }

  if (use_any_kind(TK_CONTINUE)) {
    if (!inside_roop) {
      errorf_at(ER_COMPILE, before_token, "%s",
                "continue statement not within loop");
    }
    ret = new_node(ND_CONTINUE, NULL, NULL);
    ret->lhs = inside_roop;
    use_expect_symbol(";");
    return ret;
  }

  ret = define_var();
  use_expect_symbol(";");

  return ret;
}

// define_var = base_type "*"* ident ("[" num "]")* ("=" assign)? |
//              assign
Node *define_var() {
  Type *var_type = gen_type();

  // define variable
  if (var_type) {
    int ptr_cnt = 0;
    while (use_symbol("*")) {
      ++ptr_cnt;
    }

    Token *tkn = use_any_kind(TK_IDENT);
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

    while (use_symbol("[")) {
      int array_size = use_expect_int();
      use_expect_symbol("]");

      ArraySize *now = calloc(sizeof(ArraySize), 1);
      now->array_size = array_size;
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

    if (use_symbol("=")) {
      ret = new_node(ND_ASSIGN, ret, assign());
    }
    return ret;
  }
  return assign();
}

// assign = ternary ("=" assign)?
Node *assign() {
  Node *ret = ternary();

  if (use_symbol("=")) {
    ret = new_node(ND_ASSIGN, ret, assign());
  }
  return ret;
}

// ternary = logical_or ("?" ternary ":" ternary)?
Node *ternary() {
  Node *ret = logical_or();

  if (use_symbol("?")) {
    Node *tmp = new_node(ND_TERNARY, NULL, NULL);
    tmp->lhs = ternary();
    use_expect_symbol(":");
    tmp->rhs = ternary();
    tmp->exec_if = ret;
    ret = tmp;
  }
  return ret;
}

// logical_or = logical_and ("||" logical_or)?
Node *logical_or() {
  Node *ret = logical_and();

  if (use_symbol("||")) {
    ret = new_node(ND_LOGICALOR, ret, logical_or());
  }
  return ret;
}

// logical_and = bitwise_or ("&&" logical_and)?
Node *logical_and() {
  Node *ret = bitwise_or();

  if (use_symbol("&&")) {
    ret = new_node(ND_LOGICALAND, ret, logical_and());
  }
  return ret;
}

// bitwise_or = bitwise_xor ("|" bitwise_or)?
Node *bitwise_or() {
  Node *ret = bitwise_xor();
  if (use_symbol("|")) {
    ret = new_node(ND_BITWISEOR, ret, bitwise_or());
  }
  return ret;
}

// bitwise_xor = bitwise_and ("^" bitwise_xor)?
Node *bitwise_xor() {
  Node *ret = bitwise_and();
  if (use_symbol("^")) {
    ret = new_node(ND_BITWISEXOR, ret, bitwise_xor());
  }
  return ret;
}

// bitwise_and = same_comp ("&" bitwise_and)?
Node *bitwise_and() {
  Node *ret = same_comp();
  if (use_symbol("&")) {
    ret = new_node(ND_BITWISEAND, ret, bitwise_and());
  }
  return ret;
}

// same_comp = size_comp ("==" same_comp | "!=" sama_comp)?
Node *same_comp() {
  Node *ret = size_comp();
  if (use_symbol("==")) {
    ret = new_node(ND_EQ, ret, same_comp());
  } else if (use_symbol("!=")) {
    ret = new_node(ND_NEQ, ret, same_comp());
  }
  return ret;
}

// size_comp = bitwise_shift ("<" size_comp  | ">" size_comp | "<=" size_comp | ">=" size_comp)?
Node *size_comp() {
  Node *ret = bitwise_shift();
  if (use_symbol("<")) {
    ret = new_node(ND_LC, ret, size_comp());
  } else if (use_symbol(">")) {
    ret = new_node(ND_RC, ret, size_comp());
  } else if (use_symbol("<=")) {
    ret = new_node(ND_LEC, ret, size_comp());
  } else if (use_symbol(">=")) {
    ret = new_node(ND_REC, ret, size_comp());
  }
  return ret;
}

// bitwise_shift = add ("<<" bitwise_shift | ">>" bitwise_shift)?
Node *bitwise_shift() {
  Node *ret = add();
  if (use_symbol("<<")) {
    ret = new_node(ND_LEFTSHIFT, ret, bitwise_shift());
  } else if (use_symbol(">>")) {
    ret = new_node(ND_RIGHTSHIFT, ret, bitwise_shift());
  }
  return ret;
}


// add = mul ("+" add | "-" add)?
Node *add() {
  Node *ret = mul();
  if (use_symbol("+")) {
    ret = new_node(ND_ADD, ret, add());
    raise_type_for_node(ret);
  } else if (use_symbol("-")) {
    ret = new_node(ND_SUB, ret, add());
    raise_type_for_node(ret);
  }
  return ret;
}

// mul = unary ("*" mul | "/" mul | "%" mul)?
Node *mul() {
  Node *ret = unary();
  if (use_symbol("*")) {
    ret = new_node(ND_MUL, ret, mul());
  } else if (use_symbol("/")) {
    ret = new_node(ND_DIV, ret, mul());
  } else if (use_symbol("%")) {
    ret = new_node(ND_REMAINDER, ret, mul());
  }
  return ret;
}

// unary = ("+" | "-")? address_op
Node *unary() {
  if (use_symbol("+")) {
    Node *ret = new_node(ND_ADD, new_node_int(0), priority());
    raise_type_for_node(ret);
    return ret;
  } else if (use_symbol("-")) {
    Node *ret = new_node(ND_SUB, new_node_int(0), priority());
    raise_type_for_node(ret);
    return ret;
  }
  return address_op();
}

// address_op = "&"? indirection
Node *address_op() {
  if (use_symbol("&")) {
    Node *ret = new_node(ND_ADDR, indirection(), NULL);
    return ret;
  }
  return indirection();
}

// indirection = (prefix_inc_or_dec | "*" indirection)
Node *indirection() {
  if (use_symbol("*")) {
    Node *ret = new_node(ND_CONTENT, indirection(), NULL);
    ret->var = down_type_level(ret->lhs->var);
    return ret;
  }
  return prefix_inc_or_dec();
}

// prefix_inc_or_dec = ("++" | "--")? priority
Node *prefix_inc_or_dec() {

  if (use_symbol("++")) {
    Node *ret = new_node(ND_ADD, priority(), new_node_int(1));
    raise_type_for_node(ret);
    ret = new_node(ND_ASSIGN, ret->lhs, ret);
    return ret;
  } else if (use_symbol("--")) {
    Node *ret = new_node(ND_SUB, priority(), new_node_int(1));
    raise_type_for_node(ret);
    ret = new_node(ND_ASSIGN, ret->lhs, ret);
    return ret;
  }
  return priority();
}

// priority = num |
//            "(" assign ")" |
//            ident ("(" params? ")")? |
//            ident ("[" assign "]")* |
// params = assign ("," assign)?
// base_type is gen_type()
Node *priority() {
  if (use_symbol("(")) {
    Node *ret = assign();
    use_expect_symbol(")");
    return ret;
  }

  Token *tkn = use_any_kind(TK_IDENT);

  // function call
  if (tkn) {
    if (use_symbol("(")) {
      Node *ret = new_node(ND_FUNCCALL, NULL, NULL);
      ret->func_name = tkn->str;
      ret->func_name_len = tkn->str_len;

      if (use_symbol(")")) {
        return ret;
      }

      int argc = 0;
      Node *now_arg = NULL;
      while (true) {
        Node *tmp = assign();
        tmp->func_arg = now_arg;
        now_arg = tmp;

        if (use_symbol(",")) {
          continue;
        }

        use_expect_symbol(")");
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
    while (use_symbol("[")) {
      ret = new_node(ND_ADD, ret, assign());
      ret->var = down_type_level(ret->lhs->var);

      ret = new_node(ND_CONTENT, ret, NULL);
      ret->var = ret->lhs->var;
      use_expect_symbol("]");
    }
    return ret;
  }

  return num();
}

Node *num() {
  return new_node_int(use_expect_int());
}
