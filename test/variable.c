
#include "test.h"

int main() {
  CHECK(2, ({int hoge; hoge = 2; hoge;}));
  CHECK(3, ({int test; test = 2; test = 3; test;}));
  CHECK(6, ({int foo = 2; int bar = 3; foo * bar;}));
  CHECK(5, ({int foo012 = 3; foo012 + 2;}));

  CHECK(10, ({
    int a[3];
    a[0] = 1;
    a[1] = 3;
    a[2] = 6;
    int ans = 0;
    for (int i = 0; i < 3; i = i + 1) {
      ans = ans + a[i];
    }
    ans;
  }));


  CHECK(4, ({
    int a[2][2];
    int ans = 0;
    for (int i = 0; i < 2; i = i + 1) {
      for (int j = 0; j < 2; j = j + 1) {
        a[i][j] = i + j;
        ans = ans + a[i][j];
      }
    }
    ans;
  }));

  CHECK(96, ({
    int a[2][3][4];
    int i;
    int j;
    int k;
    for (i = 0; i < 2; i = i + 1) {
      for (j = 0; j < 3; j = j + 1) {
        for (k = 0; k < 4; k = k + 1) {
          a[i][j][k] = i + j + k + 1;
        }
      }
    }
    int ans = 0;
    for (i = 0; i < 2; i = i + 1) {
      for (j = 0; j < 3; j = j + 1) {
        for (k = 0; k < 4; k = k + 1) {
          ans = ans + a[i][j][k];
        }
      }
    }
    ans;
  }));
  CHECK(5, ({
    int a = 2, b = 3;
    a + b;
  }));

  CHECK(15, ({
    int *a, b = 2;
    int c = 5, d;
    a = &d;
    d = 4;
    *a + b + c + d;
  }));

  CHECK(2, ({
    char c = 2;
    c;
  }));

  CHECK(7, ({
    char a = 2;
    int b = 5;
    a + b;

  }));

  CHECK(4, ({
    int x = 9;
    sizeof(x);
  }));

  CHECK(4, ({
    int x = 9;
    sizeof(x + 2);
  }));

  CHECK(8, ({
    long x = 4;
    sizeof(x);
  }));

  CHECK(8, ({
    int x = 3;
    int *y = &x;
    sizeof(y);
  }));

  CHECK(4, ({
    int x = 3;
    int *y = &x;
    sizeof(*y);
  }));

  CHECK(32, ({
    int x[8];
    x[2] = 4;
    sizeof(x);
  }));

  CHECK(4, ({
    int x[8];
    x[2] = 4;
    sizeof(x[2]);
  }));

  CHECK(32, ({
    long x[2][2];
    sizeof(x);
  }));


  CHECK(4, ({
    sizeof(1);
  }));

  CHECK(3, ({
    short a = 2;
    short b = 1;
    short c = a + b;
    c;
  }));

  CHECK(4, ({
    short a = 2;
    short b = 1;
    b *= a;
    a + b;
  }));

  CHECK(3, ({
    short a[2];
    a[0] = 1;
    a[1] = 2;
    a[0] + a[1];
  }));

  CHECK(12, ({
    short a[2][2];
    short b = 2;
    for (short i = 0; i < 4; ++i) {
      a[i / 2][i % 2] = i * b;
    }
    a[0][0] + a[0][1] + a[1][0] + a[1][1];
  }));

  CHECK(97, ({
    char c = 'a';
    c;
  }));

  CHECK(10, ({
    char c = '\n';
    c;
  }));

  CHECK(104, ({
    char *s = "hogehoge";
    *s;
  }));

  CHECK(111, ({
    char *s = "hoge\nhoge";
    *(s + 6);
  }));

  CHECK(5234214, ({
    long int long a = 5234214;
    a;
  }));

  CHECK(2514, ({
    short int a = 2514;
    a;
  }));

  CHECK(2514, ({
    void* a = 2514;
    a;
  }));

  CHECK(4, ({
    int a[2] = {1, 3};
    a[0] + a[1];
  }));

  CHECK(11, ({
    short a[5] = {2, 2, 1, 4, 2};
    short ans = 0;
    for (int i = 0; i < 5; i++) {
      ans += a[i];
    }
    ans;
  }));

  CHECK(16, ({
    long int a[3][2] = {{1, 3}, {3, 5}, {2, 2}};
    long int ans = 0;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 2; j++) {
        ans += a[i][j];
      }
    }
    ans;
  }));

  CHECK(135, ({
    int a[3][3][3] = {
      {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}},
      {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}},
      {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}
    };
    int ans = 0;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        for (int k = 0; k < 3; k++) {
          ans += a[i][j][k];
        }
      }
    }
    ans;
  }));

  CHECK(10, ({
    int a = 2, b = 5;
    int c[2] = {a, b + 3};
    c[0] + c[1];
  }));

  CHECK(10, ({
    int a[] = {2, 3, 5};
    a[0] + a[1] + a[2];
  }));

  CHECK(16, ({
    long int a[][2] = {{1, 3}, {3, 5}, {2, 2}};
    long int ans = 0;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 2; j++) {
        ans += a[i][j];
      }
    }
    ans;
  }));
  return 0;
}
