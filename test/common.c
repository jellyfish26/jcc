#include "test.h"
#include <stdio.h>
#include <stdlib.h>

void check(int expected, int actual, char *str) {
  if (expected == actual) {
    printf("\e[32m[ OK ]\e[m %s => %ld\n", str, actual);
  } else {
    printf("\e[31m[ NG ]\e[m %s => %ld expected but got %ld\n", str, expected, actual);
    exit(1);
  }
}
