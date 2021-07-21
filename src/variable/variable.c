#include "variable/variable.h"
#include "token/tokenize.h"

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
    case TY_ADDR:
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

Var *new_var(Type *var_type, char *str, int str_len) {
  Var *ret = calloc(sizeof(Var), 1);
  ret->var_type = var_type;
  if (ret->var_type == NULL) {
    return NULL;
  }
  ret->str = str;
  ret->len = str_len;
  return ret;
}

Var *new_content_var(Var *var) {
  if (!var->var_type->content) {
    return NULL;
  }
  Var *ret = calloc(sizeof(Var), 1);
  memcpy(ret, var, sizeof(Var));
  ret->var_type = ret->var_type->content;
  return ret;
}

Var *connect_var(Var *top_var, Type *var_type, char *str, int str_len) {
  Var *ret = new_var(var_type, str, str_len);
  ret->next = top_var;
  return ret;
}

ScopeVars *lvars;
Var *gvars;
Var *used_vars;

void new_scope_definition() {
  ScopeVars *new_scope = calloc(sizeof(ScopeVars), 1);
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
  Var *used = lvars->vars;
  while (used && used->next) {
    used = used->next;
  }
  if (used) {
    used->next = used_vars;
    used_vars = lvars->vars;
  }
  lvars = lvars->upper;
}

void add_lvar(Var *var) {
  var->global = false;
  var->next = lvars->vars;
  lvars->vars = var;
}

void add_gvar(Var *var) {
  var->global = true;
  var->next = gvars;
  if (gvars == NULL) {
    var->offset = 0;
  } else {
    if (var->var_type->kind == TY_STR) {
      var->offset = gvars->offset + 1;
    } else {
      var->offset = gvars->offset;
    }
  }
  gvars = var;
}

Var *find_var(char *str, int str_len) {
  for (ScopeVars *now_scope = lvars; now_scope; now_scope = now_scope->upper) {
    for (Var *now_var = now_scope->vars; now_var; now_var = now_var->next) {
      if (now_var->len != str_len) {
        continue;
      }

      if (memcmp(str, now_var->str, str_len) == 0) {
        return now_var;
      }
    }
  }
  for (Var *gvar = gvars; gvar; gvar = gvar->next) {
    if (gvar->len != str_len) {
      continue;
    }
    if (memcmp(str, gvar->str, str_len) == 0) {
      return gvar;
    }
  }
  return NULL;
}

bool check_already_define(char *str, int str_len, bool is_global) {
  if (is_global) {
     for (Var *gvar = gvars; gvar; gvar = gvar->next) {
      if (gvar->len != str_len) {
        continue;
      }
      if (memcmp(str, gvar->str, str_len) == 0) {
        return true;
      }
    }
    return false;
  }
  if (lvars == NULL) {
    return false;
  }
  for (Var *now_var = lvars->vars; now_var; now_var = now_var->next) {
    if (now_var->len != str_len) {
      continue;
    }

    if (memcmp(str, now_var->str, str_len) == 0) {
      return true;
    }
  }
  return false;
}

int init_offset() {
  int now_address = 0;
  Var *now = used_vars;
  while (now) {
    now_address += now->var_type->var_size;
    now->offset = now_address;
    now = now->next;
  }
  used_vars = NULL;
  now_address += 16 - (now_address % 16);
  return now_address;
}
