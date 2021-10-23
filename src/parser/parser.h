#pragma once

#include "token/tokenize.h"

#include <stdint.h>

typedef enum {
  ND_ADD,
  ND_SUB,
  ND_NUM,
} NodeKind;

typedef struct Node Node;
struct Node {
  Token *tkn;

  NodeKind kind;
  Node *lhs;
  Node *rhs;

  int64_t val;
};

Node *parser(Token *tkn);
