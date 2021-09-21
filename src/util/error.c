#include "util/util.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
    case ER_NOTE:
      err_type = "Note";
  }
  fprintf(stderr, "\x1b[31m[%s]\x1b[39m: ", err_type);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  if (type != ER_NOTE) {
    exit(1);
  }
}

void errorf_at(ERROR_TYPE type, File *file, char *loc, int underline_len, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  char *color = NULL, *cerase = "\x1b[39m\x1b[0m";
  switch (type) {
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
    }

    if (pass_loc == -1) {
      wloc++;
    } else if (pass_loc == 0) {
      strcat(code, color);
    } else if (pass_loc == underline_len) {
      strcat(code, cerase);
    }

    if (pass_loc != -1) {
      pass_loc++;
    }

    strncat(code, now_loc, 1);
    if (*now_loc == '\n' || *now_loc == '\0') {
      if (pass_loc != -1) {
        break;
      }

      hloc++;
      wloc = 1;
      memset(code, 0, 1024);
    }
  }
  fprintf(stderr, "\x1b[1m%s:%d:%d: ", file->name, hloc, wloc);

  switch (type) {
    case ER_NOTE:
      fprintf(stderr, "\x1b[34mnote:\x1b[39m\x1b[0m ");
      break;
    default:
      fprintf(stderr, "\x1b[31merror:\x1b[39m\x1b[0m ");
  }
  fprintf(stderr, fmt, ap);

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

  memset(code, '~', underline_len);
  code[0] = '^';
  code[underline_len] = '\0';
  fprintf(stderr, "%s%s%s\n", color, code, cerase);

  if (type != ER_NOTE) {
    exit(1);
  }
}

