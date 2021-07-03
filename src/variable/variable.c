#include "variable/variable.h"
#include "token/tokenize.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type *new_general_type(TypeKind kind, bool is_real) {
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

Type *new_pointer_type(Type *content_type) {
  Type *ret = new_general_type(TY_PTR, true);
  ret->content = content_type;
  content_type->is_real = false;
  return ret;
}

Type *new_array_dimension_type(Type *content_type, int dimension_size) {
  Type *ret = calloc(sizeof(Type), 1);
  ret->kind = TY_ARRAY;
  ret->is_real = false;
  ret->content = content_type;
  ret->var_size = dimension_size * content_type->var_size;
  return ret;
}

int pointer_movement_size(Type *var_type) {
  if (!var_type->content) {
    return 1;
  }
  return var_type->content->var_size;
}

Var *new_general_var(Type *var_type, char *str, int str_len) {
  Var *ret = calloc(sizeof(Var), 1);
  ret->var_type = var_type;
  if (ret->var_type == NULL) {
    return NULL;
  }
  ret->str = str;
  ret->len = str_len;
  return ret;
}

void new_pointer_var(Var *var) {
  var->var_type = new_pointer_type(var->var_type);
}

void new_array_dimension_var(Var *var, int dimension_size) {
  var->var_type = new_array_dimension_type(var->var_type, dimension_size);
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
  Var *ret = new_general_var(var_type, str, str_len);
  ret->next = top_var;
  return ret;
}

ScopeVars *local_vars;
Var *global_vars;
Var *tmp_vars;
Var *used_vars;

void new_scope_definition() {
  ScopeVars *new_scope = calloc(sizeof(ScopeVars), 1);
  if (local_vars == NULL) {
    new_scope->depth = 0;
  } else {
    new_scope->depth = local_vars->depth + 1;
  }
  new_scope->upper = local_vars;
  local_vars = new_scope;
}

void out_scope_definition() {
  if (!local_vars) {
    errorf(ER_INTERNAL, "Internal Error at scope");
  }
  Var *used = local_vars->vars;
  while (used && used->next) {
    used = used->next;
  }
  if (used) {
    used->next = used_vars;
    used_vars = local_vars->vars;
  }
  local_vars = local_vars->upper;
}

void add_local_var(Var *var) {
  var->global = false;
  var->next = local_vars->vars;
  local_vars->vars = var;
}

void add_global_var(Var *var) {
  var->global = true;
  var->next = global_vars;
  global_vars = var;
}

void add_tmp_var(Var *var) {
  var->next = tmp_vars;
  if (tmp_vars == NULL) {
    var->offset = 0;
  } else {
    var->offset = tmp_vars->offset + 1;
  }
  tmp_vars = var;
}

Var *find_var(char *str, int str_len) {
  for (Var *gvar = global_vars; gvar; gvar = gvar->next) {
    if (gvar->len != str_len) {
      continue;
    }
    if (memcmp(str, gvar->str, str_len) == 0) {
      return gvar;
    }
  }
  for (ScopeVars *now_scope = local_vars; now_scope; now_scope = now_scope->upper) {
    for (Var *now_var = now_scope->vars; now_var; now_var = now_var->next) {
      if (now_var->len != str_len) {
        continue;
      }

      if (memcmp(str, now_var->str, str_len) == 0) {
        return now_var;
      }
    }
  }
  return NULL;
}

bool check_already_define(char *str, int str_len, bool is_global) {
  if (is_global) {
     for (Var *gvar = global_vars; gvar; gvar = gvar->next) {
      if (gvar->len != str_len) {
        continue;
      }
      if (memcmp(str, gvar->str, str_len) == 0) {
        return true;
      }
    }
    return false;
  }
  if (local_vars == NULL) {
    return false;
  }
  for (Var *now_var = local_vars->vars; now_var; now_var = now_var->next) {
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
