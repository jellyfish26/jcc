#include "token/tokenize.h"
#include "read/readsrc.h"

#include <stdio.h>
#include <string.h>

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
  SourceLine *target = source_code;

  // Search
  while (target) {
    // If EOF, the source_code indicates the last line
    if (!token->str) {
      if (target->next && !target->str) {
        target = target->next;
        continue;
      } else {
        break;
      }
    }

    bool check = false;
    for (char *now = target->str; *now; now++) {
      if (now == token->str) {
        check = true;
        break;
      }
    }
    if (check) {
      break;
    }
    target = target->next;
  }

  if (!target) {
    fprintf(stderr, "Internal error\n");
  }

  fprintf(stderr, "\x1b[31m[%s]\x1b[39m\n", err_type);
  int pos = 0;
  if (token->str) {
    pos = token->str - target->str;
  } else {
    pos = target->len - 2;
  }

  // Print source code
  fprintf(stderr, "%d:%s", target->number, target->str);

  // Space of number
  for (int i = target->number; i != 0; i /= 10) {
    fprintf(stderr, " ");
  }

  // Space ": "
  for (int i = -1; i < pos + token->str_len; i++) {
    if (i < pos) {
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

Token *new_token(TokenKind kind, Token *target, char *str, int len) {
  Token *ret = calloc(1, sizeof(Token));
  // printf("%d\n", kind);
  ret->kind = kind;
  ret->str = str;
  ret->str_len = len;
  if (target) {
    target->next = ret;
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

char *permit_keywords[] = {"return", "if",       "else", "for", "while",
                           "break",  "continue", "int",  "long"};

// Update source token
void tokenize() {
  Token head;
  SourceLine *now_line = source_code;
  Token *ret = &head;

  char *now_str;
  while (now_line) {
    now_str = now_line->str;
    while (*now_str) {
      if (isspace(*now_str)) {
        now_str++;
        continue;
      }

      bool check = false;

      // Check punctuators
      for (int i = 0; i < sizeof(permit_panct) / sizeof(char *); i++) {
        int len = strlen(permit_panct[i]);
        if (memcmp(now_str, permit_panct[i], len) == 0) {
          ret = new_token(TK_PUNCT, ret, now_str, len);
          now_str += len;
          check = true;
          break;
        }
      }

      if (check) {
        continue;
      }

      // Check keywords
      for (int i = 0; i < sizeof(permit_keywords) / sizeof(char *); ++i) {
        int len = strlen(permit_keywords[i]);
        if (memcmp(now_str, permit_keywords[i], len) == 0) {
          ret = new_token(TK_KEYWORD, ret, now_str, len);
          now_str += len;
          check = true;
          break;
        }
      }

      if (check) {
        continue;
      }

      if (isdigit(*now_str)) {
        ret = new_token(TK_NUM_INT, ret, now_str, 0);
        char *tmp = now_str;
        ret->val = strtol(now_str, &now_str, 10);
        ret->str_len = now_str - tmp;
        continue;
      }

      if (is_useable_char(*now_str)) {
        char *start = now_str;
        while (is_ident_char(*now_str)) {
          now_str++;
        }
        int len = now_str - start;
        ret = new_token(TK_IDENT, ret, start, len);
        continue;
      }
      printf("%s", now_str);
      errorf(ER_TOKENIZE, "Unexpected tokenize");
    }
    now_line = now_line->next;
  }
  new_token(TK_EOF, ret, NULL, 1);
  source_token = head.next;
}
