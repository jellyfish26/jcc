#pragma once
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenize.c
//

typedef enum {
  TK_NUM_INT, // Number (Int)
  TK_PUNCT,   // Punctuators
  TK_KEYWORD, // Keywords
  TK_IDENT,   // Ident (etc. variable)
  TK_CHAR,    // Char literal
  TK_STR,     // String literal
  TK_EOF,     // End of File
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind; // Type of Token
  Token *before;  // Before token
  Token *next;    // Next token
  char *str;      // Token String
  int str_len;    // Token length
  int val;        // Value if kind is TK_NUM_INT
  char c_lit;     // Char literal
  char *str_lit;  // String literal
};

extern Token *source_token; // Warn: Don't operate it directly.
extern Token *before_token; // before source_token
Token *tokenize(char *file_name);

//
// error.c
//

typedef enum {
  ER_COMPILE,  // Compiler Error
  ER_TOKENIZE, // Tokenize Error
  ER_INTERNAL, // Internal Error
} ERROR_TYPE;

void errorf(ERROR_TYPE type, char *format, ...);
void errorf_loc(ERROR_TYPE type, char *loc, int underline_len, char *fmt, ...);
void errorf_tkn(ERROR_TYPE type, Token *tkn, char *fmt, ...);

//
// operate.c
//

bool consume(Token *ptr, Token **end_ptr, TokenKind kind, char *op);
Token *consume_old(TokenKind kind, char *op);
void restore(Token *tkn, Token **end_tkn);
void restore_old();
bool is_eof();
