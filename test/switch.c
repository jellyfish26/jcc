#include "test.h"

int main() {
  CHECK(8, ({
    int ans = 2, a = 3;
    switch (a) {
      case 1:
        ans += 3;
      case 2:
        ans += 3;
      case 3:
        ans += 3;
      case 4:
        ans += 3;
    }
    ans;
  }));

  CHECK(5, ({
    int ans = 2, a = 3;
    switch (a) {
      case 1:
        ans += 3;
        break;
      case 2:
        ans += 3;
        break;
      case 3:
        ans += 3;
        break;
      case 4:
        ans += 3;
        break;
    }
    ans;
  }));

  CHECK(7, ({
    int ans = 2, a = 3;
    switch (a) {
      case 1:
        ans += 3;
        break;
      case 2 + 1: {
        int a = 2;
        ans += a;
      }
      case 4:
        ans += 3;
    }
    ans;
  }));

  CHECK(8, ({
    int ans = 2, a = 3;
    switch (a) {
      case 1:
        ans += 3;
      case 2:
        ans += 3;
      case 3:
        ans += 3;
      default:
        ans += 3;
    }
    ans;
  }));

  CHECK(5, ({
    int ans = 2, a = 6;
    switch (a) {
      case 1:
        ans += 3;
      case 2:
        ans += 3;
      case 3:
        ans += 3;
      default:
        ans += 3;
    }
    ans;
  }));

  CHECK(8, ({
    int ans = 2, a = 0;
    switch (a) {
      case 1:
        ans += 3;
      case 2:
        ans += 3;
      default:
        ans += 3;
      case 3:
        ans += 3;
    }
    ans;
  }));

  CHECK(5, ({
    int ans = 2, a = 0;
    switch (a) {
      case 1:
        ans += 3;
        break;
      case 2:
        ans += 3;
        break;
      default:
        ans += 3;
        break;
      case 3:
        ans += 3;
        break;
    }
    ans;
  }));
  return 0;
}
