#include "test.h"

int main() {
  CHECK(3, (2, 3));
  CHECK(3, ({
    int a = 2;
    int b = (a = 3, a);
    a;
  }));

  CHECK(6, ({
    int a = 2, b;
    b = (a++, ++a, a += 2, a);
    a;
  }));

  return 0;
}
