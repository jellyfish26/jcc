#include "../read/read.h"
#include "token.h"

#include <stdio.h>
#include <string.h>

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

Token *before_token;
Token *source_token;

bool str_check(char *top_str, char *comp_str) {
  int comp_len = strlen(comp_str);
  return (strncmp(top_str, comp_str, comp_len) == 0 &&
          !is_ident_char(*(top_str + comp_len)));
}

// Update source token
void *tokenize() {
  Token head;
  SourceLine *now_line = source_code;
  Token *ret = &head;

  char *permit_symbol[] = {"*",  "/",  "(",  ")",  ";", "{", "}", "[", "]",
                           ",", "&", "<<", "<=", ">=", "==", "!=", "<", ">",
                           "=", "++", "--", "+", "-", "%"};

  char *now_str;
  while (now_line) {
    now_str = now_line->str;
    while (*now_str) {
      if (isspace(*now_str)) {
        now_str++;
        continue;
      }

      bool check = false;
      for (int i = 0; i < sizeof(permit_symbol) / sizeof(char *); i++) {
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
      if (str_check(now_str, "return")) {
        ret = new_token(TK_RETURN, ret, now_str, 6);
        now_str += 6;
        continue;
      }

      // "if" statement
      if (str_check(now_str, "if")) {
        ret = new_token(TK_IF, ret, now_str, 2);
        now_str += 2;
        continue;
      }

      // "else" statement
      if (str_check(now_str, "else")) {
        ret = new_token(TK_ELSE, ret, now_str, 4);
        now_str += 4;
        continue;
      }

      // "for" statement
      if (str_check(now_str, "for")) {
        ret = new_token(TK_FOR, ret, now_str, 3);
        now_str += 3;
        continue;
      }

      // "while" statement
      if (str_check(now_str, "while")) {
        ret = new_token(TK_WHILE, ret, now_str, 5);
        now_str += 5;
        continue;
      }

      // "break" statement
      if (str_check(now_str, "break")) {
        ret = new_token(TK_BREAK, ret, now_str, 5);
        now_str += 5;
        continue;
      }

      // "continue" statement
      if (str_check(now_str, "continue")) {
        ret = new_token(TK_CONTINUE, ret, now_str, 8);
        now_str += 8;
        continue;
      }

      // "int" type
      if (str_check(now_str, "int")) {
        ret = new_token(TK_INT, ret, now_str, 3);
        now_str += 3;
        continue;
      }

      if (str_check(now_str, "long")) {
        ret = new_token(TK_LONG, ret, now_str, 4);
        now_str += 4;
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
      printf("%s", now_str);
      errorf(ER_TOKENIZE, "Unexpected tokenize");
    }
    now_line = now_line->next;
  }
  new_token(TK_EOF, ret, NULL, 1);
  source_token = head.next;
}
