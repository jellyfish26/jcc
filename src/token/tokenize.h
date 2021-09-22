#pragma once
#include "util/util.h"

#include <stdbool.h>
#include <stdint.h>

//
// token.c
//

typedef enum {
  TK_NUM,       // Numerical literal
  TK_PUNCT,     // Punctuators
  TK_KEYWORD,   // Keywords
  TK_IDENT,     // Ident (etc. variable)
  TK_STR,       // String literal
  TK_PP,        // Preprocessor Token
  TK_EOF,       // End of File
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;  // Type of Token
  Token *next;     // Next token

  File *file;      // Belong of file
  char *loc;       // Token String
  int len;         // Token length

  char *ident;     // Identifier

  // When a macro is expanded,
  // the macro identifier disappears from the token list,
  // but the token before the macro is expanded is needed
  // to output error messages, expand the __LINE__ predefiend macro,
  // etc., so it is stored in the ref_tkn variable.
  Token *ref_tkn;

  void *ty;          // Type if kind is TK_NUM
  int64_t val;       // Value if kind is TK_NUM 
  long double fval;  // Floating-value if kind is TK_NUM

  char *strlit;  // String literal
};

bool equal(Token *tkn, char *op);
bool consume(Token *tkn, Token **end_tkn, char *op);
Token *skip(Token *tkn, char *op);
bool is_eof(Token *tkn);

Token *new_token(TokenKind kind, char *loc, int len);
char read_char(char *str, char **end_ptr);
Token *get_tail_token(Token *tkn);
char *get_ident(Token *tkn);
Token *tokenize(char *file_name);
Token *tokenize_file(File *file);
Token *tokenize_str(char *ptr, char *tokenize_end);

void errorf_tkn(ERROR_TYPE type, Token *tkn, char *fmt, ...);

//
// preprocess.c
//

void add_include_path(char *path);
Token *preprocess(Token *tkn);
