#include <stdlib.h>

#include "parser.h"

Type *gen_type() {
  Type *ret = calloc(1, sizeof(Type));

  Token *tkn = consume(TK_KEYWORD, "int");
  if (tkn) {
    ret->kind = TY_INT;
    ret->type_size = 4;
    ret->move_size = 1;
    return ret;
  }

  tkn = consume(TK_KEYWORD, "long");
  if (tkn) {
    while (consume(TK_KEYWORD, "long"));    // "long long ..."
    consume(TK_KEYWORD, "int");             // "long long int" or "long int"

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

Type *get_type_for_node(Node *target) {
  if (!target->var) {
    errorf(ER_INTERNAL, "The Node(%x) do not have variable", target);
  }
  return target->var->var_type;
}

void raise_type_for_node(Node *target) {
  if (target->lhs->var &&
        (get_type_for_node(target->lhs)->kind == TY_PTR ||
         get_type_for_node(target->lhs)->kind == TY_ARRAY)) {
      target->var = target->lhs->var;
    }

    if (target->rhs->var &&
        (get_type_for_node(target->rhs)->kind == TY_PTR ||
         get_type_for_node(target->rhs)->kind == TY_ARRAY)) {
      target->var = target->rhs->var;
    }
}
