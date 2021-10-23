#include "token/tokenize.h"
#include "util/util.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static File *cur_tokenize_file;

void verrorf_at(ErrorKind kind, File *file, char *loc, int len, char *fmt, va_list ap) {
  char *color = NULL, *cerase = "\x1b[39m\x1b[0m";
  switch (kind) {
  case ER_NOTE:
    color = "\x1b[34m\x1b[1m";
    break;
  default:
    color = "\x1b[31m\x1b[1m";
  }

  char code[1024] = {};
  int hloc = 1, wloc = 0;

  // Where char location belong line
  int pass_loc = -1;
  for (char *now_loc = file->contents; *now_loc != '\0'; now_loc++) {
    if (now_loc == loc) {
      pass_loc = 0;
      strcat(code, color);
    }

    if (pass_loc <= 0) {
      wloc++;
    } else if (pass_loc == len) {
      strcat(code, cerase);
    }

    if (pass_loc != -1) {
      pass_loc++;
    }

    if (*now_loc == '\n' || *now_loc == '\0') {
      if (pass_loc != -1) {
        strcat(code, cerase);
        strcat(code, "\n");
        break;
      }

      hloc++;
      wloc = 1;
      memset(code, 0, 1024);
    }

    strncat(code, now_loc, 1);
  }

  fprintf(stderr, "\x1b[1m%s:%d:%d: ", file->name, hloc, wloc);
  switch (kind) {
    case ER_NOTE:
      fprintf(stderr, "\x1b[34mnote:\x1b[39m\x1b[0m ");
      break;
    default:
      fprintf(stderr, "\x1b[31merror:\x1b[39m\x1b[0m ");
  }
  vfprintf(stderr, fmt, ap);

  // Prine line
  fprintf(stderr, "\n   %d | %s", hloc, code);

  if (code[strlen(code) - 1] != '\n') {
    fprintf(stderr, "\n");
  }

  // Print space of line location print
  int space_len = 4;
  for (int i = hloc; i != 0; i /= 10) {
    space_len++;
  }

  memset(code, 0, 1024);
  memset(code, ' ', space_len);
  strcat(code, "|");
  fprintf(stderr, "%s", code);

  memset(code, 0, 1024);
  memset(code, ' ', wloc);
  fprintf(stderr, "%s", code);

  memset(code, '~', len);
  code[0] = '^';
  code[len] = '\0';
  fprintf(stderr, "%s%s%s\n", color, code, cerase);

  if (kind != ER_NOTE) {
    exit(1);
  }
}

void errorf_at(ErrorKind kind, File *file, char *loc, int len, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verrorf_at(kind, file, loc, len, fmt, ap);
}

void errorf_tkn(ErrorKind kind, Token *tkn, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verrorf_at(kind, tkn->file, tkn->loc, tkn->len, fmt, ap);
}

static Token *new_token(TokenKind kind, char *loc, int len) {
  Token *tkn = calloc(1, sizeof(Token));
  tkn->kind = kind;
  tkn->loc = loc;
  tkn->len = len;
  tkn->file = cur_tokenize_file;
  return tkn;
}

bool is_eof(Token *tkn) {
  return tkn->kind == TK_EOF;
}

bool equal(Token *tkn, char *str) {
  return !is_eof(tkn) && strncmp(tkn->loc, str, strlen(str)) == 0;
}

bool consume(Token *tkn, Token **endtkn, char *str) {
  if (equal(tkn, str)) {
    *endtkn = tkn->next;
    return true;
  }

  *endtkn = tkn;
  return false;
}

Token *skip(Token *tkn, char *str) {
  if (!equal(tkn, str)) {
    errorf_tkn(ER_ERROR, tkn, "%s is expected to be here.", str);
  }
  return tkn->next;
}

static Token *tokenize_str(char *str) {
  Token head = {};
  Token *cur = &head;

  while (*str != '\0') {
    if (isspace(*str)) {
      str++;
      continue;
    }

    if (isdigit(*str)) {
      char *tail;
      int64_t val = strtol(str, &tail, 10);

      Token *tkn = new_token(TK_NUM, str, tail - str + 1);
      tkn->val = val;

      cur = cur->next = tkn;
      str = tail;
      continue;
    }

    if (ispunct(*str)) {
      static char *pancts[] = {
        "+", "-", "*", "/", "%", "(", ")", "<<", ">>", "<=", ">=",
        "<", ">", "==", "!=", "&&", "&", "^", "|"
      };

      int sz = sizeof(pancts) / sizeof(char *), idx = 0;
      for (; idx < sz; idx++) {
        if (strncmp(str, pancts[idx], strlen(pancts[idx])) == 0) {
          break;
        }
      }

      if (idx < sz) {
        int len = strlen(pancts[idx]);
        Token *tkn = new_token(TK_PANCT, str, len);
        cur = cur->next = tkn;
        str += len;
        continue;
      }
    }
    errorf_at(ER_ERROR, cur_tokenize_file, str, 1, "Unexpected tokenize");
  }

  cur->next = new_token(TK_EOF, str - 1, 1);
  return head.next;
}

Token *tokenize(char *path) {
  File *file = read_file(path);
  cur_tokenize_file = file;
  return tokenize_str(file->contents);
}
