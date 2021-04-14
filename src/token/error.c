#include "../read/read.h"
#include "token.h"

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
  case ER_OTHER:
    err_type = "Other Error";
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
  case ER_OTHER:
    err_type = "Other Error";
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
