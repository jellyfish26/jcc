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

static Node *add(Token *tkn, Token **endtkn);
static Node *num(Token *tkn, Token **endtkn);
static Node *mul(Token *tkn, Token **endtkn);

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

static Node *mul(Token *tkn, Token **endtkn) {
  Node *node = num(tkn, &tkn);

  while (equal(tkn, "*") || equal(tkn, "/") || equal(tkn, "%")) {
    NodeKind kind = ND_MUL;
    if (equal(tkn, "/")) {
      kind = ND_DIV;
    } else if (equal(tkn, "%")) {
      kind = ND_MOD;
    }

    node = new_side(kind, tkn, node, num(tkn->next, &tkn));
  }

  *endtkn = tkn;
  return node;
}

static Node *num(Token *tkn, Token **endtkn) {
  if (tkn->kind != TK_NUM) {
    errorf_tkn(ER_ERROR, tkn, "Need numerical constant");
  }

  Node *node = new_node(ND_NUM, tkn);
  node->val = tkn->val;

  *endtkn = tkn->next;
  return node;
}

Node *parser(Token *tkn) {
  return add(tkn, &tkn);
}
