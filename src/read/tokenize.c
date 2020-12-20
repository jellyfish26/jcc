#include "read.h"
#include "../write/write.h"

Token *new_token(TokenKind kind, Token *target, char *str, int len) {
    Token *ret = calloc(1, sizeof(Token));
    // printf("%d\n", kind);
    ret->kind = kind;
    ret->str = str;
    ret->str_len = len;
    if (target) {
        target->next = ret;
    }
    return ret;
}

Token *source_token;

// Update source token
void *tokenize() {
    Token head;
    SourceLine *now_line = source_code;
    Token *ret = &head;

    char *permit_symbol[] = {
        "+", "-", "*", "/", "(", ")"
    };

    while (now_line) {
        char *now_str = now_line->str;
        while (*now_str) {
            if (isspace(*now_str)) {
                now_str++;
                continue;
            }

            bool check = false;
            for (int i = 0; i < 6; i++) {
                int len = strlen(permit_symbol[i]);
                if (memcmp(now_str, permit_symbol[i], len) == 0) {
                    ret = new_token(TK_SYMBOL, ret, now_str, len);
                    now_str += len;
                    check = true;
                    break;
                }
            }

            if (check) {
                continue;
            }

            if (isdigit(*now_str)) {
                ret = new_token(TK_NUM_INT, ret, now_str, 0);
                char *tmp = now_str;
                ret->val = strtol(now_str, &now_str, 10);
                ret->str_len = now_str - tmp;
                continue;
            }

            errorf(ER_TOKENIZE, "Not defined");
        }
        now_line = now_line->next;
    }
    new_token(TK_EOF, ret, NULL, 0);
    source_token =  head.next;
}