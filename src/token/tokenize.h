#pragma once
#include "util/util.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// token.c
//

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

  char *strlit;  // String literal
};

char read_char(char *str, char **end_ptr);
Token *tokenize_file(File *file);
Token *tokenize(char *file_name);
bool is_ident_char(char c);

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

// 
// preprocess.c
//

typedef struct MacroArg MacroArg;
struct MacroArg {
  char *name;
  MacroArg *next;

  // The conv_tkn variable stores the token to be replaced and
  // is used when expanding the macro.
  Token *conv_tkn;
};

typedef struct {
  char *name;
  bool is_objlike;
  Token *conv_tkn;

  // Function-like macro
  MacroArg *args;
} Macro;

Macro *find_macro(Token *tkn);
void define_objlike_macro(char *ident, char *ptr, char **endptr);
void define_funclike_macro(char *ident, char *ptr, char **endptr);
void set_macro_args(Macro *macro, char *ptr, char **endptr);
Token *expand_macro(Token *tkn);
