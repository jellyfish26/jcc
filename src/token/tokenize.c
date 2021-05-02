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

char *permit_panct[] = {
  "(", ")", ";", "{", "}", "[", "]",
  "<<=", "<<", "<=", "<", ">>=", ">>", ">=", ">", "==", "!=",
  "=", "++", "+=", "+", "--", "-=", "-", "*=", "*",
  "/=", "/", "%=", "%", "&=", "&&", "&", "|=", "||", "|",
  "^=", "^", "?", ":", ",", "!", "~"
};

char *permit_keywords[] = {
  "return", "if", "else", "for", "while", "break", "continue",
  "int", "long"
};

// Update source token
void tokenize() {
  Token head;
  SourceLine *now_line = source_code;
  Token *ret = &head;

  char *now_str;
  while (now_line) {
    now_str = now_line->str;
    while (*now_str) {
      if (isspace(*now_str)) {
        now_str++;
        continue;
      }

      bool check = false;

      // Check punctuators
      for (int i = 0; i < sizeof(permit_panct) / sizeof(char *); i++) {
        int len = strlen(permit_panct[i]);
        if (memcmp(now_str, permit_panct[i], len) == 0) {
          ret = new_token(TK_PUNCT, ret, now_str, len);
          now_str += len;
          check = true;
          break;
        }
      }

      if (check) {
        continue;
      }

      // Check keywords
      for (int i = 0; i < sizeof(permit_keywords) / sizeof(char*); ++i) {
        int len = strlen(permit_keywords[i]);
        if (memcmp(now_str, permit_keywords[i], len) == 0) {
          ret = new_token(TK_KEYWORD, ret, now_str, len);
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
