#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void check(int expected, int actual, char *str) {
  if (expected == actual) {
    printf("\e[32m[ OK ]\e[m %s => %d\n", str, actual);
  } else {
    printf("\e[31m[ NG ]\e[m %s => %d expected but got %d\n", str, expected, actual);
    exit(1);
  }
}
