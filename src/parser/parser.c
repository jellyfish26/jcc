#include "parser/parser.h"
#include "token/tokenize.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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

static Node *stmt(Token *tkn, Token **endtkn);
static Node *expr(Token *tkn, Token **endtkn);
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
static Node *primary(Token *tkn, Token **endtkn);

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

// compound-statement:
//   "{" block-item-list? "}"
// block-item-list:
//   block-item (block-item)*
// block-item:
//   statement
static Node *compound_stmt(Token *tkn, Token **endtkn) {
  Node *node = new_node(ND_BLOCK, tkn);

  Node head = {};
  Node *cur = &head;

  tkn = skip(tkn, "{");
  while (!consume(tkn, &tkn, "}")) {
    cur = cur->next = stmt(tkn, &tkn);
  }

  node->lhs = head.next;
  *endtkn = tkn;
  return node;
}

// statement:
//   compound-statement
//   expression-statement
static Node *stmt(Token *tkn, Token **endtkn) {
  if (equal(tkn, "{")) {
    return compound_stmt(tkn, endtkn);
  }

  return expr_stmt(tkn, endtkn);
}

// expression
static Node *expr(Token *tkn, Token **endtkn) {
  return cond(tkn, endtkn);
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
//   primary-expression (("*" | "/" | "%") primary-expression)*
static Node *mul(Token *tkn, Token **endtkn) {
  Node *node = primary(tkn, &tkn);

  while (equal(tkn, "*") || equal(tkn, "/") || equal(tkn, "%")) {
    NodeKind kind = ND_MUL;
    if (equal(tkn, "/")) {
      kind = ND_DIV;
    } else if (equal(tkn, "%")) {
      kind = ND_MOD;
    }

    node = new_side(kind, tkn, node, primary(tkn->next, &tkn));
  }

  *endtkn = tkn;
  return node;
}

// constant:
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
//   constant |
//   "(" expression ")"
static Node *primary(Token *tkn, Token **endtkn) {
  if (equal(tkn, "(")) {
    Node *node = expr(tkn->next, &tkn);
    tkn = skip(tkn, ")");

    *endtkn = tkn;
    return node;
  }

  return constant(tkn, endtkn);
}

Node *parser(Token *tkn) {
  return stmt(tkn, &tkn);
}
