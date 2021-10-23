#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct Token Token;

typedef enum {
  TK_NUM,
  TK_PANCT,
  TK_EOF,
} TokenKind;

struct Token {
  TokenKind kind;
  Token *next;
  char *loc;
  int len;

  int64_t val;
};

bool equal(Token *tkn, char *str);
bool consume(Token *tkn, Token **endtkn, char *str);
Token *tokenize(char *path);
