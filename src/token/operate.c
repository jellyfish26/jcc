#include "token.h"

int use_expect_int() {
  if (source_token->kind != TK_NUM_INT) {
    errorf_at(ER_COMPILE, source_token, "Not a number");
  }
  before_token = source_token;
  int val = source_token->val;
  source_token = source_token->next;
  return val;
}

bool use_symbol(char *op) {
  if (source_token->kind != TK_SYMBOL || source_token->str_len != strlen(op) ||
      memcmp(source_token->str, op, source_token->str_len)) {
    return false;
  }
  before_token = source_token;
  source_token = source_token->next;
  return true;
}

void use_expect_symbol(char *op) {
  if (source_token->kind != TK_SYMBOL || source_token->str_len != strlen(op) ||
      memcmp(source_token->str, op, source_token->str_len)) {
    errorf_at(ER_COMPILE, source_token, "Need \"%s\" operator", op);
  }
  before_token = source_token;
  source_token = source_token->next;
}

// Warn: This function only checks the TokenKind.
Token *use_any_kind(TokenKind kind) {
  if (source_token->kind != kind) {
    return NULL;
  }
  Token *ret = source_token;
  before_token = source_token;
  source_token = source_token->next;
  return ret;
}

bool is_eof() { return source_token->kind == TK_EOF; }
