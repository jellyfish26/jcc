#include "token/tokenize.h"
#include "parser/parser.h"
#include "util/util.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static File *current_file;

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
  char *begin_line_loc = current_file->contents;

  // Where char location belong line
  for (char *now_loc = current_file->contents; now_loc != loc; now_loc++) {
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

Token *new_token(TokenKind kind, char *loc, int len) {
  Token *tkn = calloc(1, sizeof(Token));
  tkn->kind = kind;
  tkn->loc = loc;
  tkn->len = len;
  return tkn;
}

static bool streq(char *ptr, char *eq) {
  return strncmp(ptr, eq, strlen(eq)) == 0;
}

static bool strcaseeq(char *ptr, char *eq) {
  return strncasecmp(ptr, eq, strlen(eq)) == 0;
}

static bool is_useable_char(char c) {
  bool ret = false;
  ret |= ('a' <= c && c <= 'z');
  ret |= ('A' <= c && c <= 'Z');
  ret |= (c == '_');
  return ret;
}

bool is_ident_char(char c) {
  return is_useable_char(c) || ('0' <= c && c <= '9');
}

static bool str_check(char *top_str, char *comp_str) {
  int comp_len = strlen(comp_str);
  return (strncmp(top_str, comp_str, comp_len) == 0 &&
          !is_ident_char(*(top_str + comp_len)));
}

static char *permit_panct[] = {
    "(",   ")",  ";",  "{",  "}",  "[",  "]",  ".", "->", "<<=", "<<", "<=", "<",
    ">>=", ">>", ">=", ">",  "==", "!=", "=",  "++",  "+=", "+",  "--",
    "-=",  "-",  "*=", "*",  "/=", "/",  "%=", "%",   "&=", "&&", "&",
    "|=",  "||", "|",  "^=", "^",  "?",  ":",  ",",   "!",  "~", ":", "##", "#"};

static char *permit_keywords[] = {
  "return", "if", "else", "for", "do", "while", "break", "continue",
  "switch", "case", "default", "goto", "sizeof", "_Alignof",
  "signed", "unsigned", "void", "_Bool", "char", "short", "int",
  "long", "float", "double", "enum", "struct", "union",
  "auto", "const", "static", "typdef"};

static bool convert_tkn_int(Token *tkn) {
  char *ptr = tkn->loc;
  int base = 10;

  if (strcaseeq(ptr, "0x")) {
    base = 16;
  } else if (streq(ptr, "0")) {
    base = 8;
  }

  int64_t val = strtoull(ptr, &ptr, base);
  if (*ptr == '.') {
    return false;
  }

  bool suf_u = false;
  bool suf_l = false;

  if ((strcaseeq(ptr, "u") && streq(ptr + 1, "ll")) ||
      (strcaseeq(ptr, "u") && streq(ptr + 1, "LL")) ||
      (streq(ptr, "ll") && strcaseeq(ptr + 2, "u")) ||
      (streq(ptr, "LL") && strcaseeq(ptr + 2, "u"))) {
    ptr += 3;
    suf_u = suf_l = true;
  } else if (strcaseeq(ptr, "ul")) {
    ptr += 2;
    suf_u = suf_l = true;
  } else if (streq(ptr, "ll") || streq(ptr, "LL")) {
    ptr += 2;
    suf_l = true;
  } else if (strcaseeq(ptr, "l")) {
    ptr++;
    suf_l = true;
  } else if (strcaseeq(ptr, "u")) {
    ptr++;
    suf_u = true;
  }

  Type *ty = ty_i32;
  if (base == 10) {
    if (suf_u && suf_l) {
      ty = ty_u64;
    } else if (suf_l) {
      ty = ty_i64;
    } else if (suf_u) {
      ty = (val >> 32 ? ty_u64 : ty_u32);
    } else {
      ty = (val >> 31 ? ty_i64 : ty_i32);
    }
  } else {
    if (suf_u && suf_l) {
      ty = ty_u64;
    } else if (suf_l) {
      ty = (val >> 63 ? ty_u64 : ty_i64);
    } else if (suf_u) {
      ty = (val >> 32 ? ty_u64 : ty_u32);
    } else if (val >> 63) {
      ty = ty_u64;
    } else if (val >> 32) {
      ty = ty_i64;
    } else if (val >> 31) {
      ty = ty_u32;
    }
  }

  tkn->kind = TK_NUM;
  tkn->val = val;
  tkn->ty = ty;
  return true;
}

static void convert_tkn_num(Token *tkn) {
  if (convert_tkn_int(tkn)) {
    return;
  }

  char *ptr = tkn->loc;
  long double fval = strtold(ptr, &ptr);

  Type *ty = ty_f64;
  if (strcaseeq(ptr, "f")) {
    ty = ty_f32;
    ptr++;
  } else if (strcaseeq(ptr, "l")) {
    ty = ty_f80;
    ptr++;
  }

  tkn->kind = TK_NUM;
  tkn->fval = fval;
  tkn->ty = ty;
}

static char read_escaped_char(char *ptr, char **endptr) {
  *endptr = ptr + 1;
  switch (*ptr) {
    case '0': return '\0';
    case 'a': return '\a';
    case 'b': return '\b';
    case 't': return '\t';
    case 'n': return '\n';
    case 'v': return '\v';
    case 'f': return '\f';
    case 'r': return '\r';
  }
  return *ptr;
}

static char *strlit_end(char *ptr) {
  for (; *ptr != '"'; ptr++) {
    if (*ptr == '\n' || *ptr == '\0') {
      errorf_loc(ER_COMPILE, ptr, 1, "String must be closed with double quotation marks");
    }

    if (*ptr == '\\') {
      ptr++;
    }
  }
  return ptr;
}

static Token *read_strlit(char *begin, char **endptr) {
  char *end = strlit_end(begin + 1);
  char *str = calloc(end - begin, sizeof(char));

  int len = 0;
  for (char *ptr = begin + 1; ptr < end;) {
    if (*ptr == '\\') {
      str[len++] = read_escaped_char(ptr + 1, &ptr);
    } else {
      str[len++] = *ptr;
      ptr++;
    }
  }
  len++;
  end++;
  *endptr = end;

  Token *tkn = new_token(TK_STR, begin, end - begin);
  tkn->strlit = str;
  tkn->ty = array_to(ty_i8, len);
  return tkn;
}

static Token *read_charlit(char *begin, char **endptr) {
  char *ptr = begin + 1;
  char c;

  if (*ptr == '\\') {
    c = read_escaped_char(ptr + 1, &ptr);
  } else {
    c = *ptr;
    ptr++;
  }

  if (*ptr != '\'') {
    errorf_loc(ER_COMPILE, ptr, 1, "Char must be closed with single quotation marks");
  }
  ptr++;
  *endptr = ptr;

  Token *tkn = new_token(TK_NUM, begin, ptr - begin);
  tkn->val = c;
  tkn->ty = ty_i8;
  return tkn;
}

Token *get_tail_token(Token *tkn) {
  while (tkn->next != NULL) {
    tkn = tkn->next;
  }
  return tkn;
}

Token *tokenize_file(File *file, bool enable_macro) {
  Token head;
  Token *cur = &head;

  char *ptr = file->contents;
  while (*ptr) {
    if (streq(ptr, "#include")) {
      ptr += 8;
      cur->next = read_include(ptr, &ptr);
      cur = get_tail_token(cur);
      continue;
    }

    // Define macro
    if (enable_macro && streq(ptr, "#define ")) {
      ptr += 8;

      int strlen = 0;
      bool is_objlike = true;
      while ((!is_objlike && *ptr != ')') || (is_objlike && *ptr != ' ')) {
        ptr++;
        strlen++;
        is_objlike &= (*ptr != '(');
      }

      if (is_objlike) {
        define_objlike_macro(strndup(ptr - strlen, strlen), ptr + 1, &ptr);
      } else {
        define_funclike_macro(strndup(ptr - strlen, strlen + 1), ptr + 2, &ptr);
      }

      continue;
    }

    // Comment out of line
    if (streq(ptr, "//")) {
      while (*ptr != '\n') {
        ptr++;
      }
      ptr++;
      continue;
    }

    // Comment out of block
    if (streq(ptr, "/*")) {
      while (!streq(ptr, "*/")) {
        ptr++;
      }
      ptr += 2;
      continue;
    }

    // Skip space
    if (isspace(*ptr)) {
      if (cur->kind != TK_PP_SPACE) {
        cur = cur->next = new_token(TK_PP_SPACE, ptr, 1);
      }
      ptr++;
      continue;
    }

    // Numerical literal
    if (isdigit(*ptr)) {
      char *begin = ptr;
      while (isalnum(*ptr) || *ptr == '.') {
        ptr++;
      }
      cur = cur->next = new_token(TK_NUM, begin, ptr - begin);
      convert_tkn_num(cur);
      continue;
    }

    // Char literal
    if (*ptr == '\'') {
      cur = cur->next = read_charlit(ptr, &ptr);
      continue;
    }

    // String literal
    if (*ptr == '"') {
      cur = cur->next = read_strlit(ptr, &ptr);
      continue;
    }

    bool check = false;

    // Check punctuators
    for (int i = 0; i < sizeof(permit_panct) / sizeof(char *); i++) {
      int panct_len = strlen(permit_panct[i]);
      if (strncmp(ptr, permit_panct[i], panct_len) == 0) {
        cur = cur->next = new_token(TK_PUNCT, ptr, panct_len);
        ptr += panct_len;
        check = true;
        break;
      }
    }

    if (check) {
      continue;
    }

    // Check keywords
    for (int i = 0; i < sizeof(permit_keywords) / sizeof(char *); ++i) {
      int keyword_len = strlen(permit_keywords[i]);
      if (strncmp(ptr, permit_keywords[i], keyword_len) == 0 && !is_ident_char(*(ptr + keyword_len))) {
        cur = cur->next = new_token(TK_KEYWORD, ptr, keyword_len);
        ptr += keyword_len;
        check = true;
        break;
      }
    }

    if (check) {
      continue;
    }

    if (is_useable_char(*ptr)) {
      char *start = ptr;
      while (is_ident_char(*ptr)) {
        ptr++;
      }
      int ident_len = ptr - start;
      cur->next = new_token(TK_IDENT, start, ident_len);

      Macro *macro = enable_macro ? find_macro(cur->next) : NULL;
      if (macro != NULL && (*ptr == '(') != macro->is_objlike) {
        if (!macro->is_objlike) {
          set_macro_args(macro, ptr, &ptr);
        }
        cur->next = expand_macro(cur->next);
      }

      cur = get_tail_token(cur);
      continue;
    }
    errorf_loc(ER_TOKENIZE, ptr, 1, "Unexpected tokenize");
  }
  return head.next;
}

// Update source token
Token *tokenize(char *path) {
  File *file = read_file(path);
  current_file = file;

  Token *tkn = tokenize_file(file, true);
  get_tail_token(tkn)->next = new_token(TK_EOF, NULL, 1);
  tkn = delete_pp_token(tkn);

  return tkn;
}
