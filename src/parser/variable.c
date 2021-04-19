#include "parser.h"

Var *add_var(Type *var_type, char *str, int len) {
  Var *ret = calloc(1, sizeof(Var));
  ret->var_type = var_type;
  ret->str = str;
  ret->len = len;

  ret->next = exp_func->vars;
  exp_func->vars = ret;
  return ret;
}

Var *find_var(Token *target) {
  char *str = calloc(target->str_len + 1, sizeof(char));
  memcpy(str, target->str, target->str_len);
  Var *ret = NULL;
  for (Var *now = exp_func->vars; now; now = now->next) {
    if (now->len != target->str_len) {
      continue;
    }

    if (memcmp(str, now->str, target->str_len) == 0) {
      ret = now;
      break;
    }
  }
  return ret;
}

void init_offset(Function *target) {
  int now_address = 0;
  Var *now_var = target->vars;
  while (now_var) {
    now_address += now_var->var_type->type_size;
    now_var->offset = now_address;

    now_var = now_var->next;
  }

  // set 16 times
  now_address += 16 - (now_address % 16);

  target->vars_size = now_address;
}
