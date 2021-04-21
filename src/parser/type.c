#include <stdlib.h>

#include "parser.h"

Type *gen_type() {
  Type *ret = calloc(1, sizeof(Type));

  Token *tkn = use_any_kind(TK_INT);
  if (tkn) {
    ret->kind = TY_INT;
    ret->type_size = 4;
    ret->move_size = 1;
    return ret;
  }

  tkn = use_any_kind(TK_LONG);
  if (tkn) {
    while (use_any_kind(TK_LONG));    // "long long ..."
    use_any_kind(TK_INT);             // "long long int" or "long int"

    ret->kind = TY_LONG;
    ret->type_size = 8;
    ret->move_size = 1;
    return ret;
  }
  return NULL;
}

Type *connect_ptr_type(Type *before) {
  Type *ret = calloc(sizeof(Type), 1);

  ret->content = before;
  ret->kind = TY_PTR;
  ret->move_size = before->type_size;
  ret->type_size = 8;
  return ret;
}

Type *connect_array_type(Type *before, int array_size) {
  Type *ret = connect_ptr_type(before);

  ret->content = before;
  ret->kind = TY_ARRAY;
  ret->move_size = before->type_size;
  ret->type_size = array_size * before->type_size;
  return ret;
}
