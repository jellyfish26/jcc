#include "parser.h"

Type *gen_type() {
  Type *ret = calloc(1, sizeof(Type));

  Token *tkn = use_any_kind(TK_INT);
  if (tkn) {
    ret->kind = TY_INT;
    ret->type_size = 4;
    return ret;
  }

  tkn = use_any_kind(TK_LONG);
  if (tkn) {
    while (use_any_kind(TK_LONG));    // "long long ..."
    use_any_kind(TK_INT);             // "long long int" or "long int"

    ret->kind = TY_LONG;
    ret->type_size = 8;
    return ret;
  }
  return NULL;
}
