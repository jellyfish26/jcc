#include "parser/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type *ty_i8;
Type *ty_i16;
Type *ty_i32;
Type *ty_i64;

Type *new_type(TypeKind kind, int size) {
  Type *ty = calloc(2, sizeof(Type));
  ty->kind = kind;
  ty->size = size;
  return ty;
}

Type *copy_type(Type *ty) {
  Type *new_ty = calloc(1, sizeof(Type));
  memcpy(new_ty, ty, sizeof(Type));
  return new_ty;
}

void init_type() {
  ty_i8 = new_type(TY_CHAR, 1);
  ty_i16 = new_type(TY_SHORT, 2);
  ty_i32 = new_type(TY_INT, 4);
  ty_i64 = new_type(TY_LONG, 8);
}

Obj *new_obj(Type *ty, char *name) {
  Obj *obj = calloc(1, sizeof(Obj));
  obj->ty = ty;
  obj->name = name;
  return obj;
}

typedef struct Scope Scope;
struct Scope {
  Scope *up;

  HashMap var;
};

static Scope *scope = &(Scope){};
static int offset = 0;

void enter_scope() {
  Scope *sc = calloc(1, sizeof(Scope));
  sc->up = scope;
  scope = sc;
}

void leave_scope() {
  scope = scope->up;
}

bool add_var(Obj *var, bool set_offset) {
  if (hashmap_get(&(scope->var), var->name) != NULL) {
    return false;
  }
  hashmap_insert(&(scope->var), var->name, var);

  if (set_offset) {
    offset += var->ty->size;
    var->offset = offset;
  }
  return true;
}

Obj *find_var(char *name) {
  for (Scope *cur = scope; cur != NULL; cur = cur->up) {
    Obj *obj = hashmap_get(&(cur->var), name);
    if (obj != NULL) {
      return obj;
    }
  }
  return NULL;
}

int init_offset() {
  int sz = ((offset - 1) / 16 + 1) * 16;
  offset = 0;
  return sz;
}
