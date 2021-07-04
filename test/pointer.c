#include "test.h"

int main() {
  CHECK(8, ({
    int a = 2;
    int b = 5;
    int *c = &a;
    1 + *(&b) + *c;
  }));

  CHECK(5, ({
    int a = 2;
    int b = 3;
    int *c = &a;
    b + *c;
  }));

  CHECK(10, ({
    int a[3];
    *a = 1;
    *(a + 1) = 3;
    *(a + 2) = 6;
    int ans = 0;
    for (int i = 0; i < 3; i = i + 1) {
      ans = ans + *(a + i);
    }
    ans;
  }));

  CHECK(10, ({
    int a[3];
    a[0] = 1;
    a[1] = 3;
    a[2] = 6;
    int ans = 0;
    ans = ans + *(&a[0]);
    ans = ans + *(&(*(a + 1)));
    ans = ans + *(&(*(&(*(&a[2])))));
    ans;
  }));

  CHECK(15, ({
    int a[3][3];
    a[1][2] = 7;
    *(*(a + 2)) = 5;
    *(*(a) + 1) = 2;
    *(*(a + 2) + 2) = 1;
    int ans = *(*(a + 1) + 2);
    ans = ans + a[2][0];
    ans = ans + *(*(a) + 1);
    ans = ans + a[2][2];
  ans;
  }));
  return 0;
}
