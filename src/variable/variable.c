#include "variable/variable.h"
#include "token/tokenize.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type *new_general_type(TypeKind kind) {
  Type *ret = calloc(sizeof(Type), 1);
  ret->kind = kind;
  switch(kind) {
    case TY_CHAR:
      ret->var_size = 1;
      break;
    case TY_INT:
      ret->var_size = 4;
      break;
    case TY_LONG:
    case TY_PTR:
      ret->var_size = 8;
      break;
    default:
      return NULL;
  }
  return ret;
}

Type *new_pointer_type(Type *content_type) {
  Type *ret = calloc(sizeof(Type), 1);
  ret->kind = TY_PTR;
  ret->content = content_type;
  ret->var_size = 8;
  return ret;
}

Type *new_array_dimension_type(Type *content_type, int dimension_size) {
  Type *ret = calloc(sizeof(Type), 1);
  ret->kind = TY_ARRAY;
  ret->content = content_type;
  ret->var_size = dimension_size * content_type->var_size;
  return ret;
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

int pointer_movement_size(Var *var) {
  if (!var->var_type->content) {
    return 1;
  }
  return var->var_type->content->var_size;
}

int get_sizeof(Var *var) {
  return var->var_type->var_size;
}

ScopeVars *define_vars;
Var *used_vars;

void new_scope_definition() {
  ScopeVars *new_scope = calloc(sizeof(ScopeVars), 1);
  if (define_vars == NULL) {
    new_scope->depth = 0;
  } else {
    new_scope->depth = define_vars->depth + 1;
  }
  new_scope->upper = define_vars;
  define_vars = new_scope;
}

void out_scope_definition() {
  if (!define_vars) {
    errorf(ER_INTERNAL, "Internal Error at scope");
  }
  Var *used = define_vars->vars;
  while (used && used->next) {
    used = used->next;
  }
  if (used) {
    used->next = used_vars;
    used_vars = define_vars->vars;
  }
  define_vars = define_vars->upper;
}

void add_scope_var(Var *var) {
  var->global = (define_vars->depth == 0);
  var->next = define_vars->vars;
  define_vars->vars = var;
}

Var *find_var(char *str, int str_len) {
  for (ScopeVars *now_scope = define_vars; now_scope; now_scope = now_scope->upper) {
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

bool check_already_define(char *str, int str_len) {
  for (Var *now_var = define_vars->vars; now_var; now_var = now_var->next) {
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
