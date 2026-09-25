#include "test.h"

int main() {
  CHECK(0, ({ int x = 0; if (0.0) x = 1; x; }));
  CHECK(0, ({ int x = 0; if (0.0f) x = 1; x; }));
  CHECK(0, ({ int x = 0; if (0.0l) x = 1; x; }));
  CHECK(1, ({ int x = 0; if (1.0f) x = 1; x; }));
  CHECK(1, ({ int x = 0; if (1.0l) x = 1; x; }));
  CHECK(0, ({ int x = 0; if (0.0 / -1.0) x = 1; x; }));
  CHECK(1, ({ int x = 0; double nan = 0.0 / 0.0; if (nan) x = 1; x; }));
  CHECK(1, ({ int x = 0; long double nan = 0.0l / 0.0l; if (nan) x = 1; x; }));

  CHECK(2, ({
    double n = 2.0;
    int count = 0;
    while (n) {
      count++;
      n = n - 1.0;
      if (count == 4) break;
    }
    count;
  }));

  CHECK(0, ({
    float n = 0.0f;
    int count = 0;
    for (; n;) {
      count++;
      break;
    }
    count;
  }));

  CHECK(2, ({
    float n = 2.0f;
    int count = 0;
    for (; n; n = n - 1.0f) {
      count++;
      if (count == 4) break;
    }
    count;
  }));

  CHECK(1, ({
    long double n = 0.0l;
    int count = 0;
    do {
      count++;
      if (count == 4) break;
    } while (n);
    count;
  }));

  CHECK(2, 0.0 ? 1 : 2);
  CHECK(1, 1.0 ? 1 : 2);
  CHECK(2, 0.0l ? 1 : 2);
  CHECK(1, 1.0l ? 1 : 2);
  return 0;
}
