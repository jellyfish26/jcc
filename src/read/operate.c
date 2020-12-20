#include "read.h"
#include "../write/write.h"

int use_expect_int() {
    if (source_token->kind != TK_NUM_INT) {
        errorf(ER_COMPILE, "Not a number");
    }
    int val = source_token->val;
    source_token = source_token->next;
    return val;
}

bool use_symbol(char *op) {
    if (source_token->kind != TK_SYMBOL || 
        source_token->str_len != strlen(op) || 
        memcmp(source_token->str, op, source_token->str_len)) 
    {
        return false;
    }
    source_token = source_token->next;
    return true;
}

void use_expect_symbol(char *op) {
    if (source_token->kind != TK_SYMBOL || 
        source_token->str_len != strlen(op) || 
        memcmp(source_token->str, op, source_token->str_len)) 
    {
        errorf(ER_COMPILE, "Invalid operator");
    }
    source_token = source_token->next;
}