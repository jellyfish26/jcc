#pragma once

#include "token/tokenize.h"

#include <stdint.h>

typedef struct Node Node;
typedef struct Obj Obj;

//
// parse.c
//

typedef enum {
  ND_NONE,    // nothing
  ND_ADD,     // +
  ND_SUB,     // -
  ND_MUL,     // *
  ND_DIV,     // /
  ND_MOD,     // %
  ND_LSHIFT,  // <<
  ND_RSHIFT,  // >>
  ND_LCMP,    // <
  ND_LECMP,   // <=
  ND_EQ,      // ==
  ND_NEQ,     // !=
  ND_BITAND,  // &
  ND_BITXOR,  // ^
  ND_BITOR,   // |
  ND_LOGAND,  // &&
  ND_LOGOR,   // ||
  ND_COND,    // ? :
  ND_BLOCK,   // {...}
  ND_FUNC,    // int hoge(...) {...}
  ND_NUM,     // number
} NodeKind;

struct Node {
  Token *tkn;
  Node *next;

  NodeKind kind;
  Node *lhs;
  Node *rhs;

  Node *cond;

  Obj *obj;  // Variable or Function

  int64_t val;
};

Node *parser(Token *tkn);

//
// object.c
//

struct Obj {
  char *name;
  int offset;
}
;
Obj *new_obj(char *name, int len);
void enter_scope();
void leave_scope();

bool add_var(Obj *var, bool set_offset);
Obj *find_var(char *name);
int init_offset();

