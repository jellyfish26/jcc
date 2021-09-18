#include "test.h"

int main() {
  CHECK(8, ({
    int a = 2;
    int b[a];
    sizeof(b);
  }));

  CHECK(8, ({
    int a = 2;
    sizeof(int[a]);
  }));

  CHECK(3, ({
    int a = 2;
    int b[a];
    b[0] = 1, b[1] = 2;
    b[0] + b[1];
  }));

  CHECK(40, ({
    int a = 2;
    int b = a + 3;
    int foo[b][a];
    sizeof(foo);
  }));

  CHECK(40, ({
    int a = 2;
    int b = a + 3;
    sizeof(int[b][a]);
  }));

  CHECK(25, ({
    int a = 2;
    int b = a + 3;
    int foo[b][a];
    for (int i = 0; i < b; i++) {
      for (int j = 0; j < a; j++) {
        foo[i][j] = i + j;
      }
    }
    int ans = 0;
    for (int i = 0; i < b; i++) {
      for (int j = 0; j < a; j++) {
        ans += foo[i][j];
      }
    }
    ans;
  }));

  CHECK(128, ({
    int a = 2;
    int b = a + 2;
    double foo[b][2][a];
    sizeof(foo);
  }));

  CHECK(128, ({
    int a = 2;
    int b = a + 2;
    sizeof(double[b][2][a]);
  }));

  CHECKD(80.0, ({
    int a = 2;
    int b = a + 2;
    double foo[b][2][a];
    for (int i = 0; i < b; i++) {
      for (int j = 0; j < 2; j++) {
        for (int k = 0; k < a; k++) {
          foo[i][j][k] = (i + j + k) * 2.0;
        }
      }
    }
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

  return 0;
}
