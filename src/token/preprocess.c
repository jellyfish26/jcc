#include "token/tokenize.h"
#include "util/util.h"

#include <stdlib.h>

Token *concat_separate_ident_token(Token *head) {
  Token *dummy = calloc(1, sizeof(Token));
  dummy->next = head;
  head = dummy;

  while (dummy->next != NULL && dummy->next->next != NULL) {
    Token *first = dummy->next;
    Token *second = dummy->next->next;

    if (first->kind != second->kind || first->kind != TK_IDENT) {
      dummy = dummy->next;
      continue;
    }

    char *ptr = first->loc;
    char *endptr = second->loc + second->len;

    first = new_token(TK_IDENT, ptr, endptr - ptr);
    first->next = second->next;
    dummy->next = first;
  }

  return head->next;
}

Token *delete_pp_token(Token *tkn) {
  Token *before = calloc(1, sizeof(Token));
  Token *now = before->next = tkn;
  tkn = before;

  while (now != NULL) {
    if (now->kind == TK_PP) {
      before->next = now = now->next;
      continue;
    }

    before = now;
    now = now->next;
  }

  return tkn->next;
}

