#include "test.h"
#include "function_abi.h"
#include <stdio.h>

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

int sa(struct A tmp) {
  return tmp.a + tmp.b + tmp.c;
}

int sb(struct A tmp) {
  tmp.a = 2;
  return tmp.a + tmp.b + tmp.c;
}

int sc(struct B tmp) {
  int ans = 0;
  for (int i = 0; i < 6; i++) {
    ans += tmp.a[i];
  }
  return ans;
}

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

int foo(int a, int b) {
  return a + b;
}

float se(struct D tmp) {
  return tmp.a + tmp.b;
}

long double sf(struct E tmp) {
  return tmp.a + tmp.b;
}

long double sg(struct E tmp, long double a, int b) {
  return sf(tmp) + a + b;
}

float si(struct D tmp, int a, int b, float c) {
  return se(tmp) + a + b + c;
}

int sj(struct F tmp) {
  return tmp.a + tmp.b;
}

int sk(struct F tmp) {
  return tmp.a + tmp.b + tmp.c + tmp.d + tmp.e;
}

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

