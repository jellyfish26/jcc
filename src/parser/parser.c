#include "parser/parser.h"
#include "token/tokenize.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Type *to_ptr(Type *ty) {
  Type *new_ty = new_type(TY_PTR, 8);
  new_ty->base = ty;
  return new_ty;
}

static void add_type(Node *node) {
  if (node == NULL) {
    return;
  }

  add_type(node->next);
  add_type(node->lhs);
  add_type(node->rhs);
  add_type(node->cond);

  switch (node->kind) {
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_MOD:
  case ND_LSHIFT:
  case ND_RSHIFT:
  case ND_BITAND:
  case ND_BITXOR:
  case ND_BITOR:
     node->ty = node->lhs->ty;
     break;
  case ND_LCMP:
  case ND_LECMP:
  case ND_EQ:
  case ND_NEQ:
  case ND_LOGAND:
  case ND_LOGOR:
    node->ty = copy_type(ty_i8);
    break;
  case ND_COND:
    node->ty = node->lhs->ty;
    break;
  case ND_ADDR:
    node->ty = to_ptr(node->lhs->ty);
    break;
  case ND_DEREF:
    if (node->lhs->ty->base == NULL) {
      errorf_tkn(ER_ERROR, node->tkn, "Unexpected dereference");
    }

    node->ty = node->lhs->ty->base;
    break;
  case ND_RETURN:
    node->ty = node->lhs->ty;
    break;
  case ND_VAR:
    node->ty = node->obj->ty;
    break;
  case ND_NUM:
    node->ty = copy_type(ty_i32);
    break;
  case ND_GNU_STMT: {
    Type *tail;
    for (Node *stmt = node->lhs->lhs; stmt != NULL; stmt = stmt->next) {
      tail = stmt->ty;
    }
    node->ty = tail;
    return;
  }
  default:
    break;
  }
}

static Node *new_node(NodeKind kind, Token *tkn) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->tkn = tkn;
  return node;
}

static Node *new_side(NodeKind kind, Token *tkn, Node *lhs, Node *rhs) {
  Node *node = new_node(kind, tkn);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_addr(Token *tkn, Node *lhs) {
  if (lhs->kind == ND_DEREF) {
    return lhs->lhs;
  }

  return new_side(ND_ADDR, tkn, lhs, NULL);
}

static Node *new_assign(Token *tkn, Node *lhs, Node *rhs) {
  add_type(lhs);
  add_type(rhs);

  Node *node = new_node(ND_ASSIGN, tkn);
  node->lhs = new_addr(tkn, lhs);
  node->rhs = rhs;
  node->ty = lhs->ty;
  return node;
}


static Node *compound_stmt(Token *tkn, Token **endtkn);
static Node *stmt(Token *tkn, Token **endtkn);
static Obj *declarator(Token *tkn, Token **endtkn, Type *ty);
static Type *declspec(Token *tkn, Token **endtkn);
static Node *declaration(Token *tkn, Token **endtkn);
static Node *expr(Token *tkn, Token **endtkn);
static Node *assign(Token *tkn, Token **endtkn);
static Node *cond(Token *tkn, Token **endtkn);
static Node *logor(Token *tkn, Token **endtkn);
static Node *logand(Token *tkn, Token **endtkn);
static Node *bitor(Token * tkn, Token **endtkn);
static Node *bitxor(Token *tkn, Token **endtkn);
static Node *bitand(Token *tkn, Token **endtkn);
static Node *quality(Token *tkn, Token **endtkn);
static Node *relational(Token *tkn, Token **endtkn);
static Node *shift(Token *tkn, Token **endtkn);
static Node *add(Token *tkn, Token **endtkn);
static Node *num(Token *tkn, Token **endtkn);
static Node *mul(Token *tkn, Token **endtkn);
static Node *cast(Token *tkn, Token **endtkn);
static Node *unary(Token *tkn, Token **endtkn);
static Node *postfix(Token *tkn, Token **endtkn);
static Node *primary(Token *tkn, Token **endtkn);

// function-definition:
//   declaration-specifiers declarator compound-statement
static Node *funcdef(Token *tkn, Token **endtkn) {
  Type *ty = declspec(tkn, &tkn);
  Obj *obj = declarator(tkn, &tkn, ty);
  if (ty->kind != TY_FUNC) {
    errorf_tkn(ER_ERROR, tkn, "Unexpected function definition");
  }

  Node *node = new_node(ND_FUNC, tkn);
  node->obj = obj;

  node->lhs = compound_stmt(tkn, endtkn);
  obj->offset = init_offset();

  return node;
}

// expression-statement:
//   expression? ";"
static Node *expr_stmt(Token *tkn, Token **endtkn) {
  if (equal(tkn, ";")) {
    Node *node = new_node(ND_NONE, tkn);
    *endtkn = tkn->next;
    return node;
  }

  Node *node = expr(tkn, &tkn);
  tkn = skip(tkn, ";");
  *endtkn = tkn;
  return node;
}

// jump-statement:
//   "return" expression? ";"
static Node *jump_stmt(Token *tkn, Token **endtkn) {
  Node *node = new_node(ND_RETURN, tkn);
  tkn = skip(tkn, "return");

  if (!equal(tkn, ";")) {
    node->lhs = expr(tkn, &tkn);
  }
  tkn = skip(tkn, ";");

  *endtkn = tkn;
  return node;
}

// compound-statement:
//   "{" block-item-list? "}"
// block-item-list:
//   block-item (block-item)*
// block-item:
//   declaration
//   statement
static Node *compound_stmt(Token *tkn, Token **endtkn) {
  enter_scope();

  Node *node = new_node(ND_BLOCK, tkn);

  Node head = {};
  Node *cur = &head;

  tkn = skip(tkn, "{");
  while (!consume(tkn, &tkn, "}")) {
    Node *statement = NULL;
    if ((statement = declaration(tkn, &tkn)) == NULL) {
      statement = stmt(tkn, &tkn);
    }

    cur = cur->next = statement;
  }
  node->lhs = head.next;

  leave_scope();
  *endtkn = tkn;
  return node;
}

// statement:
//   compound-statement
//   expression-statement
//   jump-statement
static Node *stmt(Token *tkn, Token **endtkn) {
  if (equal(tkn, "{")) {
    return compound_stmt(tkn, endtkn);
  }

  if (equal(tkn, "return")) {
    return jump_stmt(tkn, endtkn);
  }

  return expr_stmt(tkn, endtkn);
}

// pointer:
//  ("*")*
static Type *pointer(Token *tkn, Token **endtkn, Type *ty) {
  while (equal(tkn, "*")) {
    ty = to_ptr(ty);
    tkn = tkn->next;
  }

  *endtkn = tkn;
  return ty;
}

// declarator:
//   pointer? direct-declarator
// direct-declarator:
//   identifier ("()")?
static Obj *declarator(Token *tkn, Token **endtkn, Type *ty) {
  ty = pointer(tkn, &tkn, ty);

  if (tkn->kind != TK_IDENT) {
    errorf_tkn(ER_ERROR, tkn, "Unexpected identifier");
  }

  Obj *obj = new_obj(tkn->loc, tkn->len);
  obj->ty = ty;

  if (consume(tkn->next, &tkn, "(")) {
    ty->kind = TY_FUNC;
    tkn = skip(tkn, ")");
  }

  *endtkn = tkn;
  return obj;
}

static bool is_typespec(Token *tkn) {
  return equal(tkn, "char") | equal(tkn, "short") | equal(tkn, "int") | equal(tkn, "long");
}

// declaration-specifiers:
//   type-specifier declaration-specifiers?
// type-specifier:
//   "char" | "short" | "int" | "long"
static Type *declspec(Token *tkn, Token **endtkn) {
  // We replace the type with a number and count it,
  // which makes it easier to detect duplicates and types.
  enum {
    CHAR   = 1,
    SHORT  = 1 << 2,
    INT    = 1 << 4,
    LONG   = 1 << 6,
  };

  int ty_cnt = 0;
  Type *ty;

  while (is_typespec(tkn)) {
    if (equal(tkn, "char")) {
      ty_cnt += CHAR;
    } else if (equal(tkn, "short")) {
      ty_cnt += SHORT;
    } else if (equal(tkn, "int")) {
      ty_cnt += INT;
    } else if (equal(tkn, "long")) {
      ty_cnt += LONG;
    }

    switch (ty_cnt) {
    case CHAR:
      ty = ty_i8;
      break;
    case SHORT:
      ty = ty_i16;
      break;
    case INT:
      ty = ty_i32;
      break;
    case LONG:
    case LONG + INT:
    case LONG + LONG:
    case LONG + LONG + INT:
      ty = ty_i64;
      break;
    default:
      errorf_tkn(ER_ERROR, tkn, "Invalid type");
    }

    tkn = tkn->next;
  }

  *endtkn = tkn;
  return copy_type(ty);
}

// Warn: Return nullable
// declaration:
//   declaration-specifiers init-declarator ";"
// init-declarator:
//   declarator
static Node *declaration(Token *tkn, Token **endtkn) {
  if (!is_typespec(tkn)) {
    return NULL;
  }

  Type *ty = declspec(tkn, &tkn);
  Obj *obj = declarator(tkn, &tkn, ty);
  if (!add_var(obj, true)) {
    errorf_tkn(ER_ERROR, tkn, "Variable '%s' has already declared", obj->name);
  }

  tkn = skip(tkn, ";");
  *endtkn = tkn;
  return new_node(ND_NONE, tkn);
}

// expression
static Node *expr(Token *tkn, Token **endtkn) {
  Node *node = assign(tkn, endtkn);
  add_type(node);
  return node;
}

// assignment-expression:
//   conditional-expression |
//   conditional-expression assignment-operator assignment-expression
// assignment-operator:
//   "=" | "*=" | "/=" | "%=" | "+=" | "-=" | "<<=" | ">>=" | "&=" | "^=" | "|="
static Node *assign(Token *tkn, Token **endtkn) {
  Node *node = cond(tkn, &tkn);

  if (equal(tkn, "=")) {
    return new_assign(tkn, node, assign(tkn->next, endtkn));
  }

  if (equal(tkn, "*=")) {
    return new_assign(tkn, node, new_side(ND_MUL, tkn, node, assign(tkn->next, endtkn)));
  }

  if (equal(tkn, "/=")) {
    return new_assign(tkn, node, new_side(ND_DIV, tkn, node, assign(tkn->next, endtkn)));
  }

  if (equal(tkn, "%=")) {
    return new_assign(tkn, node, new_side(ND_MOD, tkn, node, assign(tkn->next, endtkn)));
  }

  if (equal(tkn, "+=")) {
    return new_assign(tkn, node, new_side(ND_ADD, tkn, node, assign(tkn->next, endtkn)));
  }

  if (equal(tkn, "-=")) {
    return new_assign(tkn, node, new_side(ND_SUB, tkn, node, assign(tkn->next, endtkn)));
  }

  if (equal(tkn, "<<=")) {
    return new_assign(tkn, node, new_side(ND_LSHIFT, tkn, node, assign(tkn->next, endtkn)));
  }

  if (equal(tkn, ">>=")) {
    return new_assign(tkn, node, new_side(ND_RSHIFT, tkn, node, assign(tkn->next, endtkn)));
  }

  if (equal(tkn, "&=")) {
    return new_assign(tkn, node, new_side(ND_BITAND, tkn, node, assign(tkn->next, endtkn)));
  }

  if (equal(tkn, "^=")) {
    return new_assign(tkn, node, new_side(ND_BITXOR, tkn, node, assign(tkn->next, endtkn)));
  }

  if (equal(tkn, "|=")) {
    return new_assign(tkn, node, new_side(ND_BITOR, tkn, node, assign(tkn->next, endtkn)));
  }

  *endtkn = tkn;
  return node;
}

// conditional-expression:
//   logical-OR-expression |
//   logical-OR-expression "?" expression ":" conditional-expression
static Node *cond(Token *tkn, Token **endtkn) {
  Node *node = logor(tkn, &tkn);

  if (equal(tkn, "?")) {
    Node *cond_node = new_node(ND_COND, tkn);
    cond_node->lhs = expr(tkn->next, &tkn);
    tkn = skip(tkn, ":");
    cond_node->rhs = cond(tkn, &tkn);
    cond_node->cond = node;
    node = cond_node;
  }

  *endtkn = tkn;
  return node;
}

// logical-OR-expression
//   logical-AND-expression ("||" logical-AND-expression)*
static Node *logor(Token *tkn, Token **endtkn) {
  Node *node = logand(tkn, &tkn);

  while (equal(tkn, "||")) {
    node = new_side(ND_LOGOR, tkn, node, logand(tkn->next, &tkn));
  }

  *endtkn = tkn;
  return node;
}

// logical-AND-expression
//   inclusive-OR-expression ("&&" inclusive-OR-expression)*
static Node *logand(Token *tkn, Token **endtkn) {
  Node *node = bitor (tkn, &tkn);

  while (equal(tkn, "&&")) {
    node = new_side(ND_LOGAND, tkn, node, bitor (tkn->next, &tkn));
  }

  *endtkn = tkn;
  return node;
}

// inclusive-OR-expression
//   exclusive-OR-expression ("|" exclusive-OR-expression)*
static Node * bitor (Token * tkn, Token **endtkn) {
  Node *node = bitxor(tkn, &tkn);

  while (equal(tkn, "|")) {
    node = new_side(ND_BITOR, tkn, node, bitxor(tkn->next, &tkn));
  }

  *endtkn = tkn;
  return node;
}

// exclusive-OR-expression:
//   AND-expression ("^" AND-expression)*
static Node *bitxor(Token *tkn, Token **endtkn) {
  Node *node = bitand(tkn, &tkn);

  while (equal(tkn, "^")) {
    node = new_side(ND_BITXOR, tkn, node, bitand(tkn->next, &tkn));
  }

  *endtkn = tkn;
  return node;
}

// AND-expression:
//   quality-expression ("&" quality-expression)*
static Node *bitand(Token *tkn, Token **endtkn) {
  Node *node = quality(tkn, &tkn);

  while (equal(tkn, "&")) {
    node = new_side(ND_BITAND, tkn, node, quality(tkn->next, &tkn));
  }

  *endtkn = tkn;
  return node;
}

// quality-expression:
//   relational-expression (("==" | "!=") relational-expression)*
static Node *quality(Token *tkn, Token **endtkn) {
  Node *node = relational(tkn, &tkn);

  while (equal(tkn, "==") | equal(tkn, "!=")) {
    NodeKind kind = ND_EQ;
    if (equal(tkn, "!=")) {
      kind = ND_NEQ;
    }

    node = new_side(kind, tkn, node, relational(tkn->next, &tkn));
  }

  *endtkn = tkn;
  return node;
}

// relational-expression:
//   shift-expression (("<" | ">" | "<=" | ">=") shift-expression)*
static Node *relational(Token *tkn, Token **endtkn) {
  Node *node = shift(tkn, &tkn);

  while (equal(tkn, "<") | equal(tkn, ">") | equal(tkn, "<=") |
         equal(tkn, ">=")) {
    NodeKind kind = ND_LECMP;
    if (equal(tkn, "<=") | equal(tkn, ">=")) {
      kind = ND_LECMP;
    }

    if (equal(tkn, "<") | equal(tkn, "<=")) {
      node = new_side(kind, tkn, node, shift(tkn->next, &tkn));
    } else {
      node = new_side(kind, tkn, shift(tkn->next, &tkn), node);
    }
  }

  *endtkn = tkn;
  return node;
}

// shift-expression:
//   additive-expression (("<<" | ">>") additive-expression)*
static Node *shift(Token *tkn, Token **endtkn) {
  Node *node = add(tkn, &tkn);

  while (equal(tkn, "<<") | equal(tkn, ">>")) {
    NodeKind kind = ND_LSHIFT;
    if (equal(tkn, ">>")) {
      kind = ND_RSHIFT;
    }

    node = new_side(kind, tkn, node, add(tkn->next, &tkn));
  }

  *endtkn = tkn;
  return node;
}

// additive-expression:
//   multiplicative-expression (("+" | "-") multiplicative-expression)*
static Node *add(Token *tkn, Token **endtkn) {
  Node *node = mul(tkn, &tkn);

  while (equal(tkn, "+") || equal(tkn, "-")) {
    NodeKind kind = ND_ADD;
    if (equal(tkn, "-")) {
      kind = ND_SUB;
    }

    node = new_side(kind, tkn, node, mul(tkn->next, &tkn));
  }

  *endtkn = tkn;
  return node;
}

// multiplicative-expression:
//   cast-expression (("*" | "/" | "%") cast-expression)*
static Node *mul(Token *tkn, Token **endtkn) {
  Node *node = cast(tkn, &tkn);

  while (equal(tkn, "*") || equal(tkn, "/") || equal(tkn, "%")) {
    NodeKind kind = ND_MUL;
    if (equal(tkn, "/")) {
      kind = ND_DIV;
    } else if (equal(tkn, "%")) {
      kind = ND_MOD;
    }

    node = new_side(kind, tkn, node, cast(tkn->next, &tkn));
  }

  *endtkn = tkn;
  return node;
}

// cast-expression:
//   unary-expression
static Node *cast(Token *tkn, Token **endtkn) {
  return unary(tkn, endtkn);
}

// unary-expression:
//   postfix-expression |
//   ("++" | "--") postfix-expression |
//   unary-operator postfix-expression |
// unary-operator:
//   "&" | "*"
static Node *unary(Token *tkn, Token **endtkn) {
  if (equal(tkn, "++")) {
    Node *num = new_node(ND_NUM, tkn);
    num->val = 1;

    Node *node = new_side(ND_ADD, tkn, postfix(tkn->next, endtkn), num);
    node = new_assign(tkn, node->lhs, node);
    return node;
  }

  if (equal(tkn, "--")) {
    Node *num = new_node(ND_NUM, tkn);
    num->val = 1;

    Node *node = new_side(ND_SUB, tkn, postfix(tkn->next, endtkn), num);
    node = new_assign(tkn, node->lhs, node);
    return node;
  }

  if (equal(tkn, "&")) {
    Node *node = new_node(ND_ADDR, tkn);
    node->lhs = cast(tkn->next, endtkn);
    return node;
  }

  if (equal(tkn, "*")) {
    Node *node = new_node(ND_DEREF, tkn);
    node->lhs = cast(tkn->next, endtkn);
    return node;
  }

  return postfix(tkn, endtkn);
}


// postfix-expression:
//   primary-expression |
//   primary-expression ("++" | "--")
static Node *postfix(Token *tkn, Token **endtkn) {
  Node *node = primary(tkn, &tkn);

  if (equal(tkn, "++")) {
    Node *num = new_node(ND_NUM, tkn);
    num->val = 1;

    node = new_side(ND_ADD, tkn, node, num);
    node = new_assign(tkn, node->lhs, node);
    node = new_side(ND_SUB, tkn, node, num);

    *endtkn = tkn->next;
    return node;
  }

  if (equal(tkn, "--")) {
    Node *num = new_node(ND_NUM, tkn);
    num->val = 1;

    node = new_side(ND_SUB, tkn, node, num);
    node = new_assign(tkn, node->lhs, node);
    node = new_side(ND_ADD, tkn, node, num);

    *endtkn = tkn->next;
    return node;
  }

  *endtkn = tkn;
  return node;
}

static Node *constant(Token *tkn, Token **endtkn) {
  if (tkn->kind != TK_NUM) {
    errorf_tkn(ER_ERROR, tkn, "Need numerical constant");
  }

  Node *node = new_node(ND_NUM, tkn);
  node->val = tkn->val;

  *endtkn = tkn->next;
  return node;
}

// primary-expression:
//   identifier |
//   constant   |
//   "(" expression ")" |
//   "(" compound-statement ")" | -> GNU extension
static Node *primary(Token *tkn, Token **endtkn) {
  if (tkn->kind == TK_IDENT) {
    Node *node = new_node(ND_VAR, tkn);

    char *name = strndup(tkn->loc, tkn->len);
    if ((node->obj = find_var(name)) == NULL) {
      errorf_tkn(ER_ERROR, tkn, "Identifier '%s' is not found", name);
    }
    free(name);

    *endtkn = tkn->next;
    return node;
  }

  if (equal(tkn, "(") && equal(tkn->next, "{")) {
    Node *node = new_node(ND_GNU_STMT, tkn);
    node->lhs = compound_stmt(tkn->next, &tkn);
    tkn = skip(tkn, ")");

    *endtkn = tkn;
    return node;
  }

  if (equal(tkn, "(")) {
    Node *node = expr(tkn->next, &tkn);
    tkn = skip(tkn, ")");

    *endtkn = tkn;
    return node;
  }

  return constant(tkn, endtkn);
}

Node *parser(Token *tkn) {
  return funcdef(tkn, &tkn);
}
