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

static Node *bitxor(Token *tkn, Token **endtkn);
static Node *bitand(Token *tkn, Token **endtkn);
static Node *quality(Token *tkn, Token **endtkn);
static Node *relational(Token *tkn, Token **endtkn);
static Node *shift(Token *tkn, Token **endtkn);
static Node *add(Token *tkn, Token **endtkn);
static Node *num(Token *tkn, Token **endtkn);
static Node *mul(Token *tkn, Token **endtkn);
static Node *primary(Token *tkn, Token **endtkn);

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

  while (equal(tkn, "<") | equal(tkn, ">") | equal(tkn, "<=") | equal(tkn, ">=")) {
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
//   "(" shift-expression ")"
static Node *primary(Token *tkn, Token **endtkn) {
  if (equal(tkn, "(")) {
    Node *node = shift(tkn->next, &tkn);
    tkn = skip(tkn, ")");

    *endtkn = tkn;
    return node;
  }

  return constant(tkn, endtkn);
}

Node *parser(Token *tkn) {
  return bitxor(tkn, &tkn);
}
