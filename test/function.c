#include "test.h"

int add2(int a, int b) {
  return a + b;
}

int sub2(int a, int b) {
  return a - b;
}

int add6(int a, int b, int c, int d, int e, int f) {
  return a + b + c + d + e + f;
}

int add8(int a, int b, int c, int d, int e, int f, int g, int h) {
  return a + b + c + d + e + f + g + h;
}

int fib(int a) {
  if (a == 0) {
    return 0;
  }
  return fib(a - 1) + a;
}

int vv(void) {
  return 2;
}

int foo(int a, int b);
int foo();

int da(double a, int b) {
  return (int)a + b;
}

int dla(long double a, int b) {
  return (int)a + b;
}

int dlb(long double a, double b, double c, double d, double e, float g, double h, double i, double j, double k, int l) {
  return (int)a + (int)b + (int)c + (int)d + (int)e + (int)g + (int)h + (int)i + (int)j + (int)k + l;
}


int main() {
  CHECK(5, ({int a = 2, b = 3; add2(a, b);}));
  CHECK(1, ({int a = 2; int b = 3; sub2(b, a);}));
  CHECK(6, ({int a = 5, b = 3; add2(a - b, b + 1);}));
  CHECK(5, ({int a = 5, b = 2; add2(sub2(a, b), b);}));
  CHECK(21, ({
    int a = 1; int b = 2; int c = 3; int d = 4; int e = 5; int f = 6;
    add6(a, b, c, d, e, f);
  }));
  CHECK(36, ({
    int a = 1; int b = 2; int c = 3; int d = 4; int e = 5; int f = 6; int g = 7; int h = 8;
    add8(a, b, c, d, e, f, g, h);
  }));
  CHECK(55, ({fib(10);}));

  CHECK(2, vv());
  CHECK(3, foo(2, 1));
  CHECK(3, da(2.0, 1));
  CHECK(6, dla(5.0l, 1));

  CHECK(66, dlb(1.0l, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11));
  return 0;
}

int foo(int a, int b) {
  return a + b;
}

