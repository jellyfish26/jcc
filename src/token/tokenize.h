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
  TK_PP_SPACE,  // Space or NewLine (Use only preprocess) 
  TK_EOF,       // End of File
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;  // Type of Token
  Token *next;     // Next token

  File *file;      // Belong of file
  char *loc;       // Token String
  int len;         // Token length

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
Token *copy_token(Token *tkn);
bool is_ident_char(char c);
char read_char(char *str, char **end_ptr);
Token *get_tail_token(Token *tkn);
Token *tokenize_str(char *ptr, char *tokenize_end, bool enable_macro);
Token *tokenize_file(File *file, bool enable_macro);
Token *tokenize(char *file_name);

void errorf_tkn(ERROR_TYPE type, Token *tkn, char *fmt, ...);

// 
// preprocess.c
//

typedef struct MacroArg MacroArg;
struct MacroArg {
  char *name;
  MacroArg *next;

  // The conv_tkn variable stores the token to be replaced and
  // is used when expanding the macro.
  char *conv;
  int convlen;
};

typedef struct {
  char *name;
  bool is_objlike;
  Token *ref_tkn;

  char *conv;
  Token *conv_tkn;

  Token *(*macro_handler_fn)(Token *tkn);

  // Function-like macro
  MacroArg *args;
} Macro;

Macro *find_macro(Token *tkn);
void init_macro();
Token *delete_pp_token(Token *tkn);
void define_objlike_macro(char *name, char *ptr, char **endptr);
void define_funclike_macro(char *name, char *ptr, char **endptr);
void set_macro_args(Macro *macro, File *file, char *ptr, char **endptr);
Token *expand_macro(Token *tkn);
void add_include_path(char *path);
Token *read_include(char *ptr, char **endptr);
