#include "token/tokenize.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *source_str;

// If the token cannot be consumed, false is return value.
// When a token is consumed, the end_tkn variable
// will point to the next token.
bool consume(Token *tkn, Token **end_tkn, TokenKind kind, char *op) {
  if (tkn->kind != kind) {
    if (end_tkn != NULL) *end_tkn = tkn;
    return false;
  }

  // Panctuator consume or Ketword consume
  if (kind == TK_PUNCT || kind == TK_KEYWORD) {
    if (op == NULL) {
      if (end_tkn != NULL) *end_tkn = tkn;
      return false;
    }

    if (tkn->str_len != strlen(op) || memcmp(tkn->str, op, strlen(op))) {
      if (end_tkn != NULL) *end_tkn = tkn;
      return false;
    }
  }
  if (end_tkn != NULL) *end_tkn = tkn->next;
  return true;
}

bool is_eof(Token *tkn) {
  return tkn->kind == TK_EOF;
}

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

void errorf_loc(ERROR_TYPE type, char *loc, int underline_len, char *fmt, ...) {
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

void errorf_tkn(ERROR_TYPE type, Token *tkn, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  errorf_loc(type, tkn->str, tkn->str_len, fmt, ap);
}

Token *new_token(TokenKind kind, Token *connect, char *str, int str_len) {
  Token *ret = calloc(1, sizeof(Token));
  // printf("%d\n", kind);
  ret->kind = kind;
  ret->str = str;
  ret->str_len = str_len;
  if (connect) {
    connect->next = ret;
    ret->before = connect;
  }
  return ret;
}

bool is_useable_char(char c) {
  bool ret = false;
  ret |= ('a' <= c && c <= 'z');
  ret |= ('A' <= c && c <= 'Z');
  ret |= (c == '_');
  return ret;
}

bool is_ident_char(char c) {
  return is_useable_char(c) || ('0' <= c && c <= '9');
}

bool str_check(char *top_str, char *comp_str) {
  int comp_len = strlen(comp_str);
  return (strncmp(top_str, comp_str, comp_len) == 0 &&
          !is_ident_char(*(top_str + comp_len)));
}

char *permit_panct[] = {
    "(",   ")",  ";",  "{",  "}",  "[",  "]",  "<<=", "<<", "<=", "<",
    ">>=", ">>", ">=", ">",  "==", "!=", "=",  "++",  "+=", "+",  "--",
    "-=",  "-",  "*=", "*",  "/=", "/",  "%=", "%",   "&=", "&&", "&",
    "|=",  "||", "|",  "^=", "^",  "?",  ":",  ",",   "!",  "~"};

char *permit_keywords[] = {
  "return", "if", "else", "for", "while", "break", "continue",
  "sizeof", "char", "short", "int", "long"};

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

char *read_str(char *str, char **end_ptr) {
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
      ret = new_token(TK_CHAR, ret, now_str, 3);
      ret->c_lit = read_char(now_str + 1, &now_str);
      if (*now_str != '\'') {
        errorf_loc(ER_COMPILE, now_str, 1, "The char must be a single character.");
      }
      now_str++;
      continue;
    }

    if (*now_str == '"') {
      char *s_pos = now_str;
      ret = new_token(TK_STR, ret, now_str, 0);
      ret->str_lit = read_str(now_str + 1, &now_str);
      ret->str_len = (now_str - s_pos) - 2;
      if (*now_str != '"') {
        errorf_loc(ER_COMPILE, now_str, 1, "The string must end with \".");
      }
      now_str++;
      continue;
    }

    // Check keywords
    for (int i = 0; i < sizeof(permit_keywords) / sizeof(char *); ++i) {
      int keyword_len = strlen(permit_keywords[i]);
      if (memcmp(now_str, permit_keywords[i], keyword_len) == 0) {
        ret = new_token(TK_KEYWORD, ret, now_str, keyword_len);
        now_str += keyword_len;
        check = true;
        break;
      }
    }

    if (check) {
      continue;
    }

    if (isdigit(*now_str)) {
      char *tmp = now_str;
      ret = new_token(TK_NUM_INT, ret, now_str, now_str - tmp);
      ret->val = strtol(now_str, &now_str, 10);
      ret->str_len = now_str - tmp;
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
