#include "token/tokenize.h"
#include "parser/parser.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *source_str;

//
// About error output
//

void errorf(ERROR_TYPE type, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char *err_type;
  switch (type) {
    case ER_TOKENIZE:
      err_type = "Tokenize Error";
      break;
    case ER_COMPILE:
      err_type = "Compile Error";
      break;
    case ER_INTERNAL:
      err_type = "Internal Error";
      break;
  }
  fprintf(stderr, "\x1b[31m[%s]\x1b[39m: ", err_type);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

static void errorf_at(ERROR_TYPE type, char *loc, int underline_len, char *fmt, va_list ap) {
  char *err_type;
  switch (type) {
    case ER_TOKENIZE:
      err_type = "Tokenize Error";
      break;
    case ER_COMPILE:
      err_type = "Compile Error";
      break;
    case ER_INTERNAL:
      err_type = "Internal Error";
      break;
  }
  int line_loc = 1;
  char *begin_line_loc = source_str;

  // Where char location belong line
  for (char *now_loc = source_str; now_loc != loc; now_loc++) {
    if (*now_loc == '\n') {
      line_loc++;
      begin_line_loc = now_loc + 1;
    }
  }

  fprintf(stderr, "\x1b[31m[%s]\x1b[39m\n", err_type);
  // Prine line
  fprintf(stderr, "%d:", line_loc);
  for (char *now_loc = begin_line_loc; *now_loc != '\n' && *now_loc != '\0'; now_loc++) {
    fprintf(stderr, "%c", *now_loc);
  }
  fprintf(stderr, "\n");

  // Print space of line location print
  for (int i = line_loc; i != 0; i /= 10) {
    fprintf(stderr, " ");
  }
  fprintf(stderr, " ");
  for (char *now_loc = begin_line_loc;; now_loc++) {
    if (now_loc == loc) {
      for (int i = 0; i < underline_len; i++) {
        fprintf(stderr, "^");
      }
      break;
    } else {
      fprintf(stderr, " ");
    }
  }
  fprintf(stderr, " ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void errorf_loc(ERROR_TYPE type, char *loc, int underline_len, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  errorf_at(type, loc, underline_len, fmt, ap);
}

void errorf_tkn(ERROR_TYPE type, Token *tkn, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  errorf_at(type, tkn->loc, tkn->len, fmt, ap);
}

bool equal(Token *tkn, char *op) {
  return memcmp(tkn->loc, op, tkn->len) == 0 && tkn->len == strlen(op);
}

// If the token cannot be consumed, false is return value.
// When a token is consumed, the end_tkn variable
// will point to the next token.
bool consume(Token *tkn, Token **end_tkn, char *op) {
  if (equal(tkn, op)) {
    *end_tkn = tkn->next;
    return true;
  }
  *end_tkn = tkn;
  return false;
}

Token *skip(Token *tkn, char *op) {
  if (!equal(tkn, op)) {
    errorf_tkn(ER_COMPILE, tkn, "%s is expected to be here.", op);
  }
  return tkn->next;
}

bool is_eof(Token *tkn) {
  return tkn->kind == TK_EOF;
}

static Token *new_token(TokenKind kind, Token *connect, char *loc, int tkn_len) {
  Token *ret = calloc(1, sizeof(Token));
  // printf("%d\n", kind);
  ret->kind = kind;
  ret->loc = loc;
  ret->len = tkn_len;
  if (connect) {
    connect->next = ret;
  }
  return ret;
}

static bool is_useable_char(char c) {
  bool ret = false;
  ret |= ('a' <= c && c <= 'z');
  ret |= ('A' <= c && c <= 'Z');
  ret |= (c == '_');
  return ret;
}

static bool is_ident_char(char c) {
  return is_useable_char(c) || ('0' <= c && c <= '9');
}

static bool str_check(char *top_str, char *comp_str) {
  int comp_len = strlen(comp_str);
  return (strncmp(top_str, comp_str, comp_len) == 0 &&
          !is_ident_char(*(top_str + comp_len)));
}

static char *permit_panct[] = {
    "(",   ")",  ";",  "{",  "}",  "[",  "]",  "<<=", "<<", "<=", "<",
    ">>=", ">>", ">=", ">",  "==", "!=", "=",  "++",  "+=", "+",  "--",
    "-=",  "-",  "*=", "*",  "/=", "/",  "%=", "%",   "&=", "&&", "&",
    "|=",  "||", "|",  "^=", "^",  "?",  ":",  ",",   "!",  "~", ":"};

static char *permit_keywords[] = {
  "return", "if", "else", "for", "do", "while", "break", "continue",
  "switch", "case", "default", "goto", "sizeof", 
  "signed", "unsigned", "void", "_Bool", "char", "short", 
  "int", "long", "float", "double", "const"};

char read_char(char *str, char **end_ptr) {
  if (*str == 92) {
    *end_ptr = str + 2;
    switch (*(str + 1)) {
      case '0':
        return '\0';
      case 'a':
        return '\a';
      case 'b':
        return '\b';
      case 't':
        return '\t';
      case 'n':
        return '\n';
      case 'v':
        return '\v';
      case 'f':
        return '\f';
      case 'r':
        return '\r';
      default:
        return *(str + 1);
    }
  }
  *end_ptr = str + 1;
  return *str;
}

static char *read_strlit(char *str, char **end_ptr) {
  int str_len = 0;
  {
    char *now_loc = str;
    while (*now_loc != '"') {
      read_char(now_loc, &now_loc);
    }
    str_len = (int)(now_loc - str);
  }
  char *ret = calloc(str_len + 1, sizeof(char));
  memcpy(ret, str, str_len);
  *end_ptr = str + str_len;
  return ret;
}

static int hex_to_decimal(char c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  } else if ('a' <= c && c <= 'f') {
    return c - 'a' + 10;
  } else if ('A' <= c && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

// Return null, if float constant
static Token *read_integer(char *str, char **end_ptr, Token *connect) {
  Token *tkn = new_token(TK_NUM, connect, str, 0);
  int prefix = 10;

  if (memcmp(str, "0x", 2) == 0) {
    prefix = 16;
    str += 2;
  } else if (*str == '0') {
    prefix = 8;
    str += 1;
  }

  int64_t val = strtoull(str, &str, prefix);
  if (*str == '.') {
    return NULL;
  }

  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_INT;
  ty->var_size = 4;

  // Implicit type
  if (prefix == 10 && ((uint64_t)val>>31)>0) {
    ty->kind = TY_LONG;
    ty->var_size = 8;
  }

  if ((prefix == 8 || prefix == 16) && ((uint64_t)val>>30)>0) {
    ty->is_unsigned = true;
  }

  if ((prefix == 8 || prefix == 16) && ((uint64_t)val>>31)>0) {
    ty->kind = TY_LONG;
    ty->var_size = 8;
  }

  if ((prefix == 8 || prefix == 16) && ((uint64_t)val>>62)>0) {
    ty->kind = TY_LONG;
    ty->var_size = 8;
  }

  if ((prefix == 8 || prefix == 16) && ((uint64_t)val>>63)>0) {
    ty->kind = TY_LONG;
    ty->var_size = 8;
    ty->is_unsigned = true;
  }

  // Explicit type
  if (*str == 'u' || *str == 'U') {
    ty->is_unsigned = true;
    str++;
  }

  if (*str == 'l' || *str == 'L') {
    ty->kind = TY_LONG;
    ty->var_size = 8;
    str++;
  } else if (memcmp(str, "ll", 2) == 0 || memcmp(str, "LL", 2) == 0) {
    ty->kind = TY_LONG;
    ty->var_size = 8;
    str += 2;
  }

  if (*str == 'u' || *str == 'U') {
    if (ty->is_unsigned) {
      errorf_loc(ER_COMPILE, str, 1, "Invalid suffix of integer constant.");
    }
    ty->is_unsigned = true;
    str++;
  }

  tkn->val = val;
  tkn->len = str - tkn->loc;
  tkn->ty = ty;
  *end_ptr = str;
  return tkn;
}

static Token *read_float(char *str, char **end_ptr, Token *connect) {
  Token *tkn = new_token(TK_NUM, connect, str, 0);
  long double val = strtold(str, &str);

  Type *ty = ty_f64;
  if (*str == 'f' || *str == 'F') {
    ty = ty_f32;
    str++;
  } else if (*str == 'l' || *str == 'L') {
    ty = ty_f80;
    str++;
  }

  tkn->fval = val;
  tkn->len = str - tkn->loc;
  tkn->ty = ty;
  *end_ptr = str;
  return tkn;
}

// Update source token
Token *tokenize(char *file_name) {
  FILE *fp;
  if ((fp = fopen(file_name, "r")) == NULL) {
    fprintf(stderr, "Failed to open the file: %s\n", file_name);
    exit(1);
  }
  // Get file length and string
  fseek(fp, 0, SEEK_END);
  int file_len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  source_str = calloc(sizeof(char), file_len + 1);
  for (int i = 0; i < file_len; ++i) {
    *(source_str + i) = fgetc(fp);
  }
  // Insert a new line at the end
  if (*(source_str + file_len - 1) != '\n') {
    *(source_str + file_len - 1) = '\n';
  }
  fclose(fp);

  Token head;
  Token *ret = &head;

  char *now_str = source_str;
  while (*now_str) {
    // Comment out of line
    if (memcmp(now_str, "//", 2) == 0) {
      while (*now_str != '\n') {
        now_str++;
      }
      now_str++;
      continue;
    }
    
    // Comment out of block
    if (memcmp(now_str, "/*", 2) == 0) {
      while (memcmp(now_str, "*/", 2) != 0) {
        now_str++;
      }
      now_str += 2;
      continue;
    }

    if (isspace(*now_str)) {
      now_str++;
      continue;
    }

    if ((*now_str == '-' || *now_str == '+') && isdigit(*(now_str + 1))) {
      char *p;
      strtol(now_str + 1, &p, 10);
      if (*p == '.') {
        Token *tkn = read_float(now_str, &now_str, ret);
        ret = tkn;
        continue;
      }
    }

    if (isdigit(*now_str)) {
      Token *tkn = read_integer(now_str, &now_str, ret);
      if (tkn != NULL) {
        ret = tkn;
        continue;
      }

      tkn = read_float(now_str, &now_str, ret);
      if (tkn != NULL) {
        ret = tkn;
        continue;
      }
    }

    bool check = false;

    // Check punctuators
    for (int i = 0; i < sizeof(permit_panct) / sizeof(char *); i++) {
      int panct_len = strlen(permit_panct[i]);
      if (now_str + panct_len >= now_str + file_len) {
        continue;
      }
      if (memcmp(now_str, permit_panct[i], panct_len) == 0) {
        ret = new_token(TK_PUNCT, ret, now_str, panct_len);
        now_str += panct_len;
        check = true;
        break;
      }
    }

    if (check) {
      continue;
    }

    // Check char
    if (*now_str == '\'') {
      char *s_pos = now_str;
      ret = new_token(TK_NUM, ret, now_str, 3);
      ret->val = read_char(now_str + 1, &now_str);
      ret->ty = ty_i8;

      if (*now_str != '\'') {
        errorf_loc(ER_COMPILE, now_str, 1, "The char must be a single character.");
      }
      now_str++;
      continue;
    }

    if (*now_str == '"') {
      char *s_pos = now_str;
      ret = new_token(TK_STR, ret, now_str, 0);
      ret->str_lit = read_strlit(now_str + 1, &now_str);
      ret->len = (now_str - s_pos) - 2;
      if (*now_str != '"') {
        errorf_loc(ER_COMPILE, now_str, 1, "The string must end with \".");
      }
      now_str++;
      continue;
    }

    // Check keywords
    for (int i = 0; i < sizeof(permit_keywords) / sizeof(char *); ++i) {
      int keyword_len = strlen(permit_keywords[i]);
      if (memcmp(now_str, permit_keywords[i], keyword_len) == 0 && *(now_str + keyword_len + 1) == ' ') {
        ret = new_token(TK_KEYWORD, ret, now_str, keyword_len);
        now_str += keyword_len;
        check = true;
        break;
      }
    }

    if (check) {
      continue;
    }

    if (is_useable_char(*now_str)) {
      char *start = now_str;
      while (is_ident_char(*now_str)) {
        now_str++;
      }
      int ident_len = now_str - start;
      ret = new_token(TK_IDENT, ret, start, ident_len);
      continue;
    }
    printf("%s", now_str);
    errorf_loc(ER_TOKENIZE, now_str, 1, "Unexpected tokenize");
  }
  new_token(TK_EOF, ret, NULL, 1);
  return head.next;
}
