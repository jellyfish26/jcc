#include "parser/parser.h"

#include <stdlib.h>
#include <string.h>

Obj *new_obj(char *name, int len) {
  Obj *obj = calloc(1, sizeof(Obj));
  obj->name = strndup(name, len);
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
    offset += 4;
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
  int sz = offset;
  offset = 0;
  return sz;
}
