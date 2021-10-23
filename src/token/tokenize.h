#pragma once
#include "util/util.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  TK_NUM,
  TK_PANCT,
  TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;

  File *file;
  char *loc;
  int len;

  int64_t val;
};

bool equal(Token *tkn, char *str);
bool consume(Token *tkn, Token **endtkn, char *str);
Token *tokenize(char *path);

typedef enum {
  ER_ERROR,
  ER_NOTE,
} ErrorKind;

void verrorf_at(ErrorKind kind, File *file, char *loc, int len, char *fmt, va_list ap);
void errorf_at(ErrorKind kind, File *file, char *loc, int len, char *fmt, ...);
void errorf_tkn(ErrorKind kind, Token *tkn, char *fmt, ...);
