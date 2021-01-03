#include "token.h"
#include "../read/read.h"

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

bool is_useable_char(char c) {
    bool ret = false;
    ret |= ('a' <= c && c <= 'z');
    ret |= ('A' <= c && c <= 'Z');
    ret |= (c == '_');
    return ret;
}

bool is_ident_char(char c) {
    return is_useable_char(c) || ('0' <= c && c <= '9');
}

Token *source_token;

// Update source token
void *tokenize() {
    Token head;
    SourceLine *now_line = source_code;
    Token *ret = &head;

    char *permit_symbol[] = {
        "+", "-", "*", "/", "(", ")", ";", "{", "}",
        "<=", ">=", "==", "!=", "<", ">", "=" 
    };

    char *now_str;
    while (now_line) {
        now_str = now_line->str;
        while (*now_str) {
            if (isspace(*now_str)) {
                now_str++;
                continue;
            }

            bool check = false;
            for (int i = 0; i < sizeof(permit_symbol) / sizeof(char*); i++) {
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

            // "return" statement
            if (strncmp(now_str, "return", 6) == 0 && !is_ident_char(*(now_str + 6))) {
                ret = new_token(TK_RETURN, ret, now_str, 6);
                now_str += 6;
                continue;
            }

            // "if" statement
            if (strncmp(now_str, "if", 2) == 0 && !is_ident_char(*(now_str + 2))) {
                ret = new_token(TK_IF, ret, now_str, 2);
                now_str += 2;
                continue;
            }

            // "else" statement
            if (strncmp(now_str, "else", 4) == 0 && !is_ident_char(*(now_str + 4))) {
                ret = new_token(TK_ELSE, ret, now_str, 4);
                now_str += 4;
                continue;
            }

            // "for" statement
            if (strncmp(now_str, "for", 3) == 0 && !is_ident_char(*(now_str + 3))) {
                ret = new_token(TK_FOR, ret, now_str, 3);
                now_str += 3;
                continue;
            }

            if (isdigit(*now_str)) {
                ret = new_token(TK_NUM_INT, ret, now_str, 0);
                char *tmp = now_str;
                ret->val = strtol(now_str, &now_str, 10);
                ret->str_len = now_str - tmp;
                continue;
            }

            if (is_useable_char(*now_str)) {
                char *start = now_str;
                while (is_ident_char(*now_str)) {
                    now_str++;
                }
                int len = now_str - start;
                ret = new_token(TK_IDENT, ret, start, len);
                continue;
            }

            errorf(ER_TOKENIZE, "Unexpected tokenize");
        }
        now_line = now_line->next;
    }
    new_token(TK_EOF, ret, NULL, 1);
    source_token =  head.next;
}