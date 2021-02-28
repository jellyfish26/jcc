#include "parser.h"

Var *add_var(VarKind kind, char *str, int len) {
    Var *ret = calloc(1, sizeof(Var));
    ret->kind = kind;
    ret->str = str;
    ret->len = len;

    ret->size = 8;
    exp_func->vars_size += 8;

    ret->next = exp_func->vars;
    exp_func->vars = ret;
    return ret;
}

Var *find_var(Token *target) {
    char *str = calloc(target->str_len + 1, sizeof(char));
    memcpy(str, target->str, target->str_len);
    Var *ret = NULL;
    for (Var *now = exp_func->vars; now; now = now->next) {
        if (memcmp(str, now->str, target->str_len) == 0) {
            ret = now;
            break;
        }
    }
    return ret;
}

void init_offset(Var *now_var) {
    int now_address = 0;
    while (now_var) {
        now_address += now_var->size;
        now_var->offset = now_address;

        now_var = now_var->next;
    }
}