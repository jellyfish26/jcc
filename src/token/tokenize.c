#include "token/tokenize.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *source_str;

// If the token cannot be consumed, NULL is return value.
// When a token is consumed, the source_token variable
// will point to the next token.
Token *consume(TokenKind kind, char *op) {
  if (source_token->kind != kind) {
    return NULL;
  }

  // Panctuator consume or Keyword consume
  if (kind == TK_PUNCT || kind == TK_KEYWORD) {
    if (op == NULL) {
      return NULL;
    }

    if (source_token->str_len != strlen(op) ||
        memcmp(source_token->str, op, strlen(op))) {
      return NULL;
    }
  }

  // Move token
  Token *ret = source_token;
  before_token = source_token;
  source_token = source_token->next;
  return ret;
}

void restore() {
  if (before_token != NULL) {
    before_token = before_token->before;
  }
  source_token = source_token->before;
}

bool is_eof() {
  return source_token->kind == TK_EOF;
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

void errorf_at(ERROR_TYPE type, Token *token, char *fmt, ...) {
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
  int line_cnt = 1;
  int begin_line_loc = token->loc;
  char *now_str = source_str;

  // Explore where the Token is on the line.
  while (now_str < source_str + token->loc) {
    if (*now_str == '\n') {
      line_cnt++;
    }
    now_str++;
  }

  // Go back to the beginning of the line
  while (now_str != source_str && *(now_str - 1) != '\n') {
    now_str--;
    begin_line_loc--;
  }

  fprintf(stderr, "\x1b[31m[%s]\x1b[39m\n", err_type);

  // Print source code
  fprintf(stderr, "%d:", line_cnt);
  while (*now_str != '\n' && *now_str != '\0') {
    fprintf(stderr, "%c", *now_str);
    now_str++;
  }
  fprintf(stderr, "\n");

  // Space of number
  for (int i = line_cnt; i != 0; i /= 10) {
    fprintf(stderr, " ");
  }

  // Space ": "
  for (int i = begin_line_loc - 1; i < token->loc + token->str_len; i++) {
    if (i < token->loc) {
      fprintf(stderr, " ");
    } else {
      fprintf(stderr, "^");
    }
  }
  fprintf(stderr, " ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

Token *new_token(TokenKind kind, Token *connect, char *str, int len, int loc) {
  Token *ret = calloc(1, sizeof(Token));
  // printf("%d\n", kind);
  ret->kind = kind;
  ret->str = str;
  ret->str_len = len;
  ret->loc = loc;
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

Token *before_token;
Token *source_token;

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
  "sizeof", "char", "int", "long"};

// Update source token
void tokenize(char *file_name) {
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
  fclose(fp);

  Token head;
  Token *ret = &head;

  char *now_str = source_str;
  int now_loc = 0;
  while (*now_str) {
    if (isspace(*now_str)) {
      now_str++;
      now_loc++;
      continue;
    }

    bool check = false;

    // Check punctuators
    for (int i = 0; i < sizeof(permit_panct) / sizeof(char *); i++) {
      int panct_len = strlen(permit_panct[i]);
      if (memcmp(now_str, permit_panct[i], panct_len) == 0) {
        ret = new_token(TK_PUNCT, ret, now_str, panct_len, now_loc);
        now_str += panct_len;
        now_loc += panct_len;
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
      if (memcmp(now_str, permit_keywords[i], keyword_len) == 0) {
        ret = new_token(TK_KEYWORD, ret, now_str, keyword_len, now_loc);
        now_str += keyword_len;
        now_loc += keyword_len;
        check = true;
        break;
      }
    }

    if (check) {
      continue;
    }

    if (isdigit(*now_str)) {
      char *tmp = now_str;
      ret = new_token(TK_NUM_INT, ret, now_str, now_str - tmp, now_loc);
      ret->val = strtol(now_str, &now_str, 10);
      ret->str_len = now_str - tmp;
      now_loc += ret->str_len;
      continue;
    }

    if (is_useable_char(*now_str)) {
      char *start = now_str;
      while (is_ident_char(*now_str)) {
        now_str++;
      }
      int ident_len = now_str - start;
      ret = new_token(TK_IDENT, ret, start, ident_len, now_loc);
      now_loc += ident_len;
      continue;
    }
    printf("%s", now_str);
    errorf(ER_TOKENIZE, "Unexpected tokenize");
  }
  new_token(TK_EOF, ret, NULL, 1, now_loc);
  source_token = head.next;
}
