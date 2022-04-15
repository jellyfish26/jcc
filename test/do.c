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

  return 0;
}
