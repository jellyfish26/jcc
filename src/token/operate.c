#include "token.h"
#include <string.h>

// If the token cannot be consumed, NULL is return value.
// When a token is consumed, the source_token variable
// will point to the next token.
Token *consume(TokenKind kind, char *op) {
  if (source_token->kind != kind) {
    return NULL;
  }

  // Panctuator consume or Keyword consume
  if (kind == TK_PUNCT || kind == TK_KEYWORD) {
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

