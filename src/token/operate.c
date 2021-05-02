#include "token.h"
#include <string.h>

// If the token cannot be consumed, NULL is return value.
// When a token is consumed, the source_token variable
// will point to the next token.
Token *consume(TokenKind kind, char *op) {
  if (source_token->kind != kind) {
    return NULL;
  }

  // Symbol consume
  if (kind == TK_SYMBOL) {
    if (op == NULL) {
      return NULL;
    }

    if (source_token->str_len != strlen(op) || memcmp(source_token->str, op, strlen(op))) {
      return NULL;
    }
  }

  // Move token
  Token *ret = source_token;
  before_token = source_token;
  source_token = source_token->next;
  return ret;
}

bool is_eof() {
  return source_token->kind == TK_EOF;
}

