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

void arr3(int a[2][5]) {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 5; j++) {
      a[i][j] = i + j;
    }
  }
}

int arr3_cnt(int a[2][5]) {
  int ans = 0;
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 5; j++) {
      ans += a[i][j];
    }
  }
  return ans;
}

struct D {
  int a;
  float b;
};

float se(struct D tmp) {
  return tmp.a + tmp.b;
}

struct E {
  char a;
  long double b;
};

long double sf(struct E tmp) {
  return tmp.a + tmp.b;
}

long double sg(struct E tmp, long double a, int b) {
  return sf(tmp) + a + b;
}

float si(struct D tmp, int a, int b, float c) {
  return se(tmp) + a + b + c;
}

struct F {
  char a: 2;
  int b: 30;
  char c: 2, d: 2;
  short e: 12;
};

int sj(struct F tmp) {
  return tmp.a + tmp.b;
}

int sk(struct F tmp) {
  return tmp.a + tmp.b + tmp.c + tmp.d + tmp.e;
}

union G {
  int a: 3;
  int b: 8;
};

int ua(union G tmp) {
  return tmp.a + tmp.b;
}

int instatic() {
  static int a = 0;
  return ++a;
}

int inauto() {
  auto int a = 0;
  return ++a;
}

static int fnstatic() {
  int a = 2;
  return a++;
}

struct A retsta() {
  struct A tmp = {1, 2, 3};
  return tmp;
}

struct B retstb() {
  struct B tmp = {{1, 2, 3, 4, 5, 6}};
  return tmp;
}

struct C retstc() {
  struct C tmp = {2.2l};
  return tmp;
}

struct D retstd() {
  struct D tmp = {2, 3.0f};
  return tmp;
}

struct E retste() {
  struct E tmp = {2, 3.0l};
  return tmp;
}

struct F retstf() {
  struct F tmp = {2, 50, 1, -1, 240};
  return tmp;
}

void ivla_1(int a, int b[a]) {
  b[0] = 1;
  b[1] = 2;
}

int ivla_1cnt(int a, int b[a]) {
  int ans = 0;
  for (int i = 0; i < a; i++) {
    ans += b[i];
  }
  return ans;
}

void ivla_2(int a, int b, int c[b][a]) {
  for (int i = 0; i < b; i++) {
    for (int j = 0; j < a; j++) {
      c[i][j] = i + j;
    }
  }
}

int ivla_2cnt(int a, int b, int c[b][a]) {
  int ans = 0;
  for (int i = 0; i < b; i++) {
    for (int j = 0; j < a; j++) {
      ans += c[i][j];
    }
  }
  return ans;
}

void dvla_1(int a, int b, double c[b][2][a]) {
  for (int i = 0; i < b; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < a; k++) {
        c[i][j][k] = (i + j + k) * 2.0;
      }
    }
  }
}

double dvla_1cnt(int a, int b, double c[b][2][a]) {
  double ans = 0;
  for (int i = 0; i < b; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < a; k++) {
        ans += c[i][j][k];
      }
    }
  }
  return ans;
}

int ivla_3(int a, int b[a][a]) {
  int before = sizeof(b[0]);
  a = 5;
  int after = sizeof(b[0]);
  return after - before;
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

  CHECK(6, ({
    struct A tmp = {2, 3, 1};
    sa(tmp);
  }));

  CHECK(2, ({
    struct A tmp = {4, 3, 1};
    int ans = tmp.a + tmp.b + tmp.c;
    ans - sb(tmp);
  }));

  CHECK(21, ({
    struct B tmp = {{1, 2, 3, 4, 5, 6}};
    sc(tmp);
  }));

  CHECKLD(3.0, ({
    struct C tmp = {3.0};
    sd(tmp);
  }));

  CHECK(15, ({
    int tmp[5] = {1, 2, 3, 4, 5};
    arr1(tmp);
  }));

  CHECK(0, ({
    int tmp[] = {1, 2, 3, 4, 5};
    int ans = arr2(tmp);
    for (int i = 0; i < 5; i++) {
      ans -= tmp[i];
    }
    ans;
  }));

  CHECK(25, ({
    int foo[2][5];
    arr3(foo);
    int ans = 0;
    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 5; j++) {
        ans += foo[i][j];
      }
    }
    ans;
  }));

  CHECK(25, ({
    int foo[2][5];
    arr3(foo);
    arr3_cnt(foo);
  }));

  CHECKF(5.0, ({
    struct D tmp = {2, 3.0f};
    se(tmp);
  }));

  CHECKLD(4.1l, ({
    struct E tmp = {1, 3.1l};
    sf(tmp);
  }));

  CHECKLD(9.1l, ({
    struct E tmp = {1, 3.1l};
    sg(tmp, 2.0l, 3);
  }));

  CHECKF(13.0f, ({
    struct D tmp = {2, 3.0f};
    si(tmp, 2, 3, 3.0f);
  }));

  CHECK(48, ({
    struct F tmp;
    tmp.a = 2;
    tmp.b = 50;
    sj(tmp);
  }));

  CHECK(288, ({
    struct F tmp;
    tmp.a = 2;
    tmp.b = 50;
    tmp.c = 1;
    tmp.d = -1;
    tmp.e = 240;
    sk(tmp);
  }));

  CHECK(14, ({
    union G tmp;
    tmp.b = 11;
    ua(tmp);
  }));

  CHECK(10, ({
    union G tmp;
    tmp.b = 11;
    tmp.a = -3;
    ua(tmp);
  }));

  CHECK(3, ({
    instatic();
    instatic();
    instatic();
  }));

  CHECK(1, ({
    inauto();
    inauto();
    inauto();
  }));

  CHECK(2, ({
    fnstatic();
    fnstatic();
    fnstatic();
  }));

  CHECK(6, ({
    struct A tmp = retsta();
    tmp.a + tmp.b + tmp.c;
  }));

  CHECK(6, ({
    struct A tmp;
    tmp = retsta();
    tmp.a + tmp.b + tmp.c;
  }));

  CHECK(21, ({
    struct B tmp = retstb();
    int ans = 0;
    for (int i = 0; i < 6; i++) {
      ans += tmp.a[i];
    }
    ans;
  }));

  CHECK(21, ({
    struct B tmp;
    tmp = retstb();
    int ans = 0;
    for (int i = 0; i < 6; i++) {
      ans += tmp.a[i];
    }
    ans;
  }));

  CHECKLD(2.2L, ({
    struct C tmp = retstc();
    tmp.a;
  }));

  CHECKLD(2.2L, ({
    struct C tmp;
    tmp = retstc();
    tmp.a;
  }));

  CHECKF(5.0f, ({
    struct D tmp = retstd();
    tmp.a + tmp.b;
  }));

  CHECKF(5.0f, ({
    struct D tmp;
    tmp = retstd();
    tmp.a + tmp.b;
  }));

  CHECKLD(5.0l, ({
    struct E tmp = retste();
    tmp.a + tmp.b;
  }));

  CHECKLD(5.0l, ({
    struct E tmp;
    tmp = retste();
    tmp.a + tmp.b;
  }));

  CHECK(288, ({
    struct F tmp = retstf();
    tmp.a + tmp.b + tmp.c + tmp.d + tmp.e;
  }));

  CHECK(288, ({
    struct F tmp;
    tmp = retstf();
    tmp.a + tmp.b + tmp.c + tmp.d + tmp.e;
  }));

  CHECK(36, ({
    int a = 1; int b = 2; int c = 3; int d = 4; int e = 5; int f = 6; int g = 7; int h = 8;
    int ans = add8(a, b, c, d, e, f, g, h);
    ans;
  }));

  CHECK(10, ({
    struct B {
      struct A aa;
      int bb;
    };
    struct B tmp = {retsta(), add2(2, 2)};
    tmp.aa.a + tmp.aa.b + tmp.aa.c + tmp.bb;
  }));

  CHECK(3, ({
    int a = 2;
    int b[2];
    ivla_1(a, b);
    b[0] + b[1];
  }));

  CHECK(3, ({
    int a = 2;
    int b[2];
    ivla_1(a, b);
    ivla_1cnt(a, b);
  }));

  CHECK(25, ({
    int a = 2;
    int b = a + 3;
    int foo[b][a];
    ivla_2(a, b, foo);
    int ans = 0;
    for (int i = 0; i < b; i++) {
      for (int j = 0; j < a; j++) {
        ans += foo[i][j];
      }
    }
    ans;
  }));

  CHECK(25, ({
    int a = 2;
    int b = a + 3;
    int foo[b][a];
    ivla_2(a, b, foo);
    ivla_2cnt(a, b, foo);
  }));

  CHECKD(80.0, ({
    int a = 2;
    int b = a + 2;
    double foo[b][2][a];
    dvla_1(a, b, foo);
    double ans = 0;
    for (int i = 0; i < b; i++) {
      for (int j = 0; j < 2; j++) {
        for (int k = 0; k < a; k++) {
          ans += foo[i][j][k];
        }
      }
    }
    ans;
  }));

  CHECKD(80.0, ({
    int a = 2;
    int b = a + 2;
    double foo[b][2][a];
    dvla_1(a, b, foo);
    dvla_1cnt(a, b, foo);
  }));

  CHECK(0, ({
    int a = 2;
    int foo[a][a];
    ivla_3(a, foo);
  }));

  CHECK(2, ({
    int (a) = 2;
    a;
  }));

  CHECK(7, ({
    int (((((((a))))))) = 5, (b) = 2;
    a + b;
  }));

  return 0;
}

int foo(int a, int b) {
  return a + b;
}

