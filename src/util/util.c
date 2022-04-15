#include "util/util.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// In the C language, there were no characters from backslash to newline.
// For example, "abc\ \ndef" becomes "abcdef".
char *erase_bslash_str(char *ptr, int len) {
  char *str = calloc(len + 1, sizeof(char));

  bool ignore_char = false;
  char *endptr = ptr + len;
  for (; ptr != endptr; ptr++) {
    if (ignore_char && *ptr == '\n') {
      ignore_char = false;
      continue;
    }

    if (*ptr == '\\') {
      ignore_char = true;
    }

    if (ignore_char) {
      continue;
    }

    strncat(str, ptr, 1);
  }

  return str;
}

bool isident(char c) {
  return isalpha(c) || isdigit(c) || (c == '_');
}
