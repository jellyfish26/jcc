#include "token/tokenize.h"
#include "parser/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type *new_type(TypeKind kind, bool is_real) {
  Type *ret = calloc(sizeof(Type), 1);
  ret->kind = kind;
  ret->is_real = is_real;
  switch(kind) {
    case TY_CHAR:
      ret->var_size = 1;
      break;
    case TY_SHORT:
      ret->var_size = 2;
      break;
    case TY_INT:
      ret->var_size = 4;
      break;
    case TY_STR:
    case TY_LONG:
    case TY_PTR:
      ret->var_size = 8;
      break;
    default:
      return NULL;
  }
  return ret;
}

Type *pointer_to(Type *type) {
  Type *ret = new_type(TY_PTR, true);
  ret->content = type;
  type->is_real = false;
  return ret;
}

Type *array_to(Type *type, int dim_size) {
  Type *ret = calloc(1, sizeof(Type));
  ret->kind = TY_ARRAY;
  ret->is_real = false;
  ret->content = type;
  ret->var_size = dim_size * type->var_size;
  return ret;
}

Obj *new_obj(Type *type, char *name) {
  int name_len = strlen(name);
  Obj *ret = calloc(1, sizeof(Obj));
  ret->type = type;
  if (ret->type == NULL) {
    return NULL;
  }
  ret->name = name;
  ret->name_len = name_len;
  return ret;
}

ScopeObj *lvars;
Obj *gvars;
Obj *used_vars;

void new_scope_definition() {
  ScopeObj *new_scope = calloc(1, sizeof(ScopeObj));
  if (lvars == NULL) {
    new_scope->depth = 0;
  } else {
    new_scope->depth = lvars->depth + 1;
  }
  new_scope->upper = lvars;
  lvars = new_scope;
}

void out_scope_definition() {
  if (!lvars) {
    errorf(ER_INTERNAL, "Internal Error at scope");
  }
  Obj *used = lvars->objs;
  while (used && used->next) {
    used = used->next;
  }
  if (used) {
    used->next = used_vars;
    used_vars = lvars->objs;
  }
  lvars = lvars->upper;
}

void add_lvar(Obj *var) {
  var->is_global = false;
  var->next = lvars->objs;
  lvars->objs = var;
}

void add_gvar(Obj *var) {
  var->is_global = true;
  var->next = gvars;
  if (gvars == NULL) {
    var->offset = 0;
  } else {
    if (var->type->kind == TY_STR) {
      var->offset = gvars->offset + 1;
    } else {
      var->offset = gvars->offset;
    }
  }
  gvars = var;
}

Obj *find_var(char *name) {
  int name_len = strlen(name);
  // Find in local variable
  for (ScopeObj *now_scope = lvars; now_scope != NULL; now_scope = now_scope->upper) {
    for (Obj *obj = now_scope->objs; obj != NULL; obj = obj->next) {
      if (name_len != obj->name_len) {
        continue;
      }

      if (memcmp(name, obj->name, name_len) == 0) {
        return obj;
      }
    }
  }

  // Find in global varaible
  for (Obj *obj = gvars; obj != NULL; obj = obj->next) {
    if (name_len != obj->name_len) {
      continue;
    }

    if (memcmp(name, obj->name, name_len) == 0) {
      return obj;
    }
  }
  return NULL;
}


int init_offset() {
  int now_address = 0;
  Obj *now = used_vars;
  while (now) {
    now_address += now->type->var_size;
    now->offset = now_address;
    now = now->next;
  }
  used_vars = NULL;
  now_address += 16 - (now_address % 16);
  return now_address;
}

bool check_already_define(char *name, bool is_global) {
  int name_len = strlen(name);
  if (is_global) {
     for (Obj *gvar = gvars; gvar != NULL; gvar = gvar->next) {
      if (gvar->name_len != name_len) {
        continue;
      }
      if (memcmp(name, gvar->name, name_len) == 0) {
        return true;
      }
    }
    return false;
  }
  if (lvars == NULL) {
    return false;
  }
  for (Obj *now_var = lvars->objs; now_var != NULL; now_var = now_var->next) {
    if (now_var->name_len != name_len) {
      continue;
    }

    if (memcmp(name, now_var->name, name_len) == 0) {
      return true;
    }
  }
  return false;
}
