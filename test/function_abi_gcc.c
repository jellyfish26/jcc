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

struct A {
  int a, b;
  long c;
};

int sa(struct A tmp) {
  return tmp.a + tmp.b + tmp.c;
}

int sb(struct A tmp) {
  tmp.a = 2;
  return tmp.a + tmp.b + tmp.c;
}

struct B {
  int a[6];
};

int sc(struct B tmp) {
  int ans = 0;
  for (int i = 0; i < 6; i++) {
    ans += tmp.a[i];
  }
  return ans;
}

struct C {
  long double a;
};

long double sd(struct C tmp) {
  return tmp.a;
}

int arr1(int a[5]) {
  int ans = 0;
  for (int i = 0; i < 5; i++) {
    ans += a[i];
  }
  return ans;
}

int arr2(int a[]) {
  int ans = 0;
  a[1] = 2;
  a[3] = 5;
  for (int i = 0; i < 5; i++) {
    ans += a[i];
  }
  return ans;
}

int foo(int a, int b) {
  return a + b;
}

