#include "test.h"

int a;
int b;
int c, d;
int ar[2][2];
short s;

int main() {
  CHECK(2, ({a = 2; a;}));
  CHECK(5, ({b = 1; int *c = &a; *c = 4; a + b;}));
  CHECK(4, ({
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 2; ++j) {
        ar[i][j] = i + j;
      }
    }
    ar[0][0] + ar[1][1] + ar[0][1] + ar[1][0];
  }));
  CHECK(5, ({short a = 2; s = a + 3; s;}));
  CHECK(10, ({
    short a = 2;
    s = a + 3;
    // a = 5;
    /* c = 8; */
    s * a;
  }));
  return 0;
}

