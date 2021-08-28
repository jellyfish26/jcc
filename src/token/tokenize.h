#pragma once
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TK_NUM,     // Numerical literal
  TK_PUNCT,   // Punctuators
  TK_KEYWORD, // Keywords
  TK_IDENT,   // Ident (etc. variable)
  TK_STR,     // String literal
  TK_EOF,     // End of File
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;  // Type of Token
  Token *next;     // Next token

  char *loc;       // Token String
  int len;         // Token length

  void *ty;          // Type if kind is TK_NUM
  int64_t val;       // Value if kind is TK_NUM 
  long double fval;  // Floating-value if kind is TK_NUM

  char *str_lit;   // String literal
};

char read_char(char *str, char **end_ptr);
Token *tokenize(char *file_name);

typedef enum {
  ER_COMPILE,  // Compiler Error
  ER_TOKENIZE, // Tokenize Error
  ER_INTERNAL, // Internal Error
} ERROR_TYPE;

void errorf(ERROR_TYPE type, char *format, ...);
void errorf_loc(ERROR_TYPE type, char *loc, int underline_len, char *fmt, ...);
void errorf_tkn(ERROR_TYPE type, Token *tkn, char *fmt, ...);

bool equal(Token *tkn, char *op);
bool consume(Token *tkn, Token **end_tkn, char *op);
Token *skip(Token *tkn, char *op);
bool is_eof(Token *tkn);
