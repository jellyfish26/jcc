#pragma once

#include "token/tokenize.h"

#include <stdint.h>

typedef struct Node Node;
typedef struct Type Type;
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
  ND_ASSIGN,  // =
  ND_ADDR,    // &
  ND_DEREF,   // *
  ND_BLOCK,   // {...}
  ND_FUNC,    // int hoge(...) {...}
  ND_RETURN,  // "return"
  ND_VAR,     // variable
  ND_NUM,     // number
} NodeKind;

struct Node {
  Token *tkn;
  Node *next;

  NodeKind kind;
  Type *ty;
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

typedef enum {
  TY_CHAR,
  TY_SHORT,
  TY_INT,
  TY_LONG,
  TY_PTR,
  TY_FUNC,
} TypeKind;

struct Type {
  TypeKind kind;
  int size;

  Type *base; // pointer
};

extern Type *ty_i8;
extern Type *ty_i16;
extern Type *ty_i32;
extern Type *ty_i64;

Type *new_type(TypeKind kind, int size);
Type *copy_type(Type *ty);
void init_type();

struct Obj {
  Type *ty;

  char *name;
  int offset;
};

Obj *new_obj(char *name, int len);
void enter_scope();
void leave_scope();

bool add_var(Obj *var, bool set_offset);
Obj *find_var(char *name);
int init_offset();

