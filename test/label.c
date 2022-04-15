#include "test.h"

int foo() {
  int ans = 2;
  if (ans == 2) {
    goto A;
  }
  return 2;
A:
  return 3;
}

int bar() {
  int ans = 0;
A:
  if (ans < 10) {
    ans++;
    goto A;
  }
  return ans;
}

int hoge() {
  int A = -1;
B:
  A += 1;
A:
  if (A < 15) {
    A++;
    goto A;
  }
  if (A < 30) {
    goto B;
  }
  return A;
}

int main() {
  CHECK(3, foo());
  CHECK(10, bar());
  CHECK(30, hoge());

  return 0;
}
