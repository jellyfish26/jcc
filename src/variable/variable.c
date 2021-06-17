#include "variable/variable.h"

#include <stdlib.h>
#include <string.h>

Type *new_general_type(TypeKind kind) {
  Type *ret = calloc(sizeof(Type), 1);
  switch(kind) {
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

Var *find_var(Var *top_var, char *str, int str_len) {
  char *var_name = calloc(str_len + 1, sizeof(char));
  memcpy(var_name, str, str_len);
  for (Var *now = top_var; now; now = now->next) {
    if (now->len != str_len) {
      continue;
    }

    if (memcmp(var_name, now->str, str_len) == 0) {
      return now;
    }
  }
  return NULL;
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

int init_offset(Var *top_var) {
  int now_address = 0;
  Var *now = top_var;
  while (now) {
    now_address += now->var_type->var_size;
    now->offset = now_address;
    now = now->next;
  }
  now_address += 16 - (now_address % 16);
  return now_address;
}
