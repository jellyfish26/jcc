#include "test.h"
#include <stdio.h>
#include <stdlib.h>

void check(int expected, int actual, char *str) {
  if (expected == actual) {
    printf("\e[32m[ OK ]\e[m %s => %d\n", str, actual);
  } else {
    printf("\e[31m[ NG ]\e[m %s => %d expected but got %d\n", str, expected, actual);
    exit(1);
  }
}

void checkl(long expected, long actual, char *str) {
  if (expected == actual) {
    printf("\e[32m[ OK ]\e[m %s => %ld\n", str, actual);
  } else {
    printf("\e[31m[ NG ]\e[m %s => %ld expected but got %ld\n", str, expected, actual);
    exit(1);
  }
}

void checkul(unsigned long expected, unsigned long actual, char *str) {
  if (expected == actual) {
    printf("\e[32m[ OK ]\e[m %s => %lu\n", str, actual);
  } else {
    printf("\e[31m[ NG ]\e[m %s => %lu expected but got %lu\n", str, expected, actual);
    exit(1);
  }
}

void checkf(float expected, float actual, char *str) {
  if (expected == actual) {
    printf("\e[32m[ OK ]\e[m %s => %f\n", str, actual);
  } else {
    printf("\e[31m[ NG ]\e[m %s => %f expected but got %f\n", str, expected, actual);
    exit(1);
  }
}

void checkd(double expected, double actual, char *str) {
  if (expected == actual) {
    printf("\e[32m[ OK ]\e[m %s => %lf\n", str, actual);
  } else {
    printf("\e[31m[ NG ]\e[m %s => %lf expected but got %lf\n", str, expected, actual);
    exit(1);
  }
}
