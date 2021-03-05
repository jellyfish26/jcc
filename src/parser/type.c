#include "parser.h"

Type *gen_type() {
    Type *ret = calloc(1, sizeof(Type));

    Token *tkn = use_any_kind(TK_INT);
    if (tkn) {
        ret->kind = TY_INT;
        ret->type_size = 8; // now implement int is long 
        return ret;
    }
    return NULL;
}