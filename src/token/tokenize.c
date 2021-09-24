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

void errorf_tkn(ERROR_TYPE type, Token *tkn, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  if (tkn->ref_tkn != NULL) {
    errorf_tkn(ER_NOTE, tkn->ref_tkn, "in expansion of macro");
  }

  verrorf_at(type, tkn->file, tkn->loc, tkn->len, fmt, ap);
}

bool equal(Token *tkn, char *op) {
  if (tkn->kind == TK_EOF) {
    errorf_tkn(ER_COMPILE, tkn, "Reached EOF");
  }

  char *str = erase_bslash_str(tkn->loc, tkn->len);
  return strcmp(str, op) == 0;
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

// The file variable contains information about the file
// in which the tokenize_file function was executed.
Token *new_token(TokenKind kind, char *loc, int len) {
  Token *tkn = calloc(1, sizeof(Token));
  tkn->kind = kind;
  tkn->file = current_file;
  tkn->loc = loc;
  tkn->len = len;
  return tkn;
}

Token *copy_token(Token *tkn) {
  Token *cpy = calloc(1, sizeof(Token));
  memcpy(cpy, tkn, sizeof(Token));

  return cpy;
}

static bool streq(char *ptr, char *eq) {
  return strncmp(ptr, eq, strlen(eq)) == 0;
}

static bool strcaseeq(char *ptr, char *eq) {
  return strncasecmp(ptr, eq, strlen(eq)) == 0;
}

static bool str_check(char *top_str, char *comp_str) {
  int comp_len = strlen(comp_str);
  return strncmp(top_str, comp_str, comp_len) == 0 && !isident(*(top_str + comp_len));
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
  "auto", "const", "static", "typedef"};

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
      errorf_at(ER_COMPILE, current_file, ptr, 1, "String must be closed with double quotation marks");
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
    errorf_at(ER_COMPILE, current_file, ptr, 1, "Char must be closed with single quotation marks");
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

void add_eof_token(Token *head) {
  Token *tail = get_tail_token(head);
  tail->next = new_token(TK_EOF, tail->loc + strlen(tail->loc) - 1, 1);
  tail->next->file = tail->file;
}

char *get_ident(Token *tkn) {
  if (tkn->kind != TK_IDENT) {
    errorf_tkn(ER_COMPILE, tkn, "Expected an identifier");
  }
  return erase_bslash_str(tkn->loc, tkn->len);
}

Token *tokenize_str(char *ptr, char *tokenize_end) {
  Token head;
  Token *cur = &head;

  while (*ptr != '\0' && ptr != tokenize_end) {
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

    // Back slash
    if (*ptr == '\\') {
      while (*ptr != '\n') {
        ptr++;
      }
      ptr++;
      continue;
    }

    // Skip space
    if (isspace(*ptr)) {
      cur = cur->next = new_token(TK_PP, ptr, 1);
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
      if (strncmp(ptr, permit_keywords[i], keyword_len) == 0 && !isident(*(ptr + keyword_len))) {
        cur = cur->next = new_token(TK_KEYWORD, ptr, keyword_len);
        ptr += keyword_len;
        check = true;
        break;
      }
    }

    if (check) {
      continue;
    }

    if (isident(*ptr) && !isdigit(*ptr)) {
      int len = 0;
      while (isident(*ptr)) {
        ptr++;
        len++;
      }

      cur = cur->next = new_token(TK_IDENT, ptr - len, len);
      continue;
    }

    errorf_at(ER_TOKENIZE, current_file, ptr, 1, "Unexpected tokenize");
  }

  return head.next;
}

Token *tokenize_file(File *file) {
  File *store_file = current_file;
  current_file = file;

  Token *tkn = tokenize_str(file->contents, NULL);

  current_file = store_file;
  return tkn;
}

// Update source token
Token *tokenize(char *path) {
  init_macro();
  File *file = read_file(path);

  Token *tkn = tokenize_file(file);
  add_eof_token(tkn);

  return preprocess(tkn);
}
