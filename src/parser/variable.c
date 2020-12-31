#include "parser.h"

Var *vars = NULL;
int vars_size;

Var *add_var(VarKind kind, Var *target, char *str, int len) {
    Var *ret = calloc(1, sizeof(Var));
    ret->kind = kind;
    ret->str = str;
    ret->len = len;

    ret->size = 8;
    vars_size += 8;

    ret->next = target;
    return ret;
}

Var *find_var(Token *target) {
    char *str = calloc(target->str_len + 1, sizeof(char));
    memcpy(str, target->str, target->str_len);
    Var *ret = NULL;
    for (Var *now = vars; now; now = now->next) {
        if (memcmp(str, now->str, target->str_len) == 0) {
            ret = now;
            break;
        }
    }
    return ret;
}

void init_offset() {
    int now_address = 0;
    Var *now_var = vars;
    while (now_var) {
        now_address += now_var->size;
        now_var->offset = now_address;

        now_var = now_var->next;
    }
}