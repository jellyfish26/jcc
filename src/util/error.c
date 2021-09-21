#include "util/util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


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

void errorf_at(ERROR_TYPE type, File *file, char *loc, int underline_len, char *fmt, ...) {
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
  char *begin_line_loc = file->contents;

  // Where char location belong line
  for (char *now_loc = file->contents; now_loc != loc; now_loc++) {
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

