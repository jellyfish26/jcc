#include "token/tokenize.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token *new_token(TokenKind kind, char *loc, int len) {
  Token *tkn = calloc(1, sizeof(Token));
  tkn->kind = kind;
  tkn->loc = loc;
  tkn->len = len;
  return tkn;
}

bool equal(Token *tkn, char *str) {
  if (tkn->kind == TK_EOF) {
    return false;
  }

  return strncmp(tkn->loc, str, strlen(str)) == 0;
}

bool consume(Token *tkn, Token **endtkn, char *str) {
  if (equal(tkn, str)) {
    *endtkn = tkn->next;
    return true;
  }

  *endtkn = tkn;
  return false;
}

Token *tokenize(char *str) {
  Token head = {};
  Token *cur = &head;

  while (*str != '\0') {
    if (isspace(*str)) {
      str++;
      continue;
    }

    if (isdigit(*str)) {
      char *tail;
      int64_t val = strtol(str, &tail, 10);

      Token *tkn = new_token(TK_NUM, str, tail - str + 1);
      tkn->val = val;

      cur = cur->next = tkn;
      str = tail;
      continue;
    }

    if (ispunct(*str)) {
      static char *pancts[] = {
        "+", "-", "*", "/"
      };

      int sz = sizeof(pancts) / sizeof(char *), idx = 0;
      for (; idx < sz; idx++) {
        if (strncmp(str, pancts[idx], strlen(pancts[idx])) == 0) {
          break;
        }
      }

      if (idx < sz) {
        int len = strlen(pancts[idx]);
        Token *tkn = new_token(TK_PANCT, str, len);
        cur = cur->next = tkn;
        str += len;
        continue;
      }

      fprintf(stderr, "Unexpected tokenize");
    }
  }

  cur->next = new_token(TK_EOF, str, 1);
  return head.next;
}
