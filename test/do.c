#include "test.h"

int main() {
  CHECK(2, ({
    int i = 0, ans = 0;
    do {
      i++;
      ans += 2;
    } while (i < 0);
    ans;
  }));

  CHECK(10, ({
    int i = 0, ans = 0;
    do {
      i++;
      ans += 2;
    } while (i < 5);
    ans;
  }));

  CHECK(1, ({
    int i = 0, checks = 0;
    do {
      i++;
      if (i == 1) continue;
    } while (++checks < 1);
    i;
  }));

  return 0;
}
