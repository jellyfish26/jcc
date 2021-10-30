#include "test.h"

int main() {
  CHECK(5, 2 + 3);
  CHECK(3, 5 - 2);
  CHECK(4, 2 * 2);
  CHECK(4, 8 / 2);
  CHECK(5, 2 * 2 + 1 * 1);
  CHECK(10, 2 * 2 / 2 + 8 / 2 + 6 - 2);
  CHECK(5, (2 + 3) * (3 - 2));
  CHECK(14, ((2 * 3 - 2) / 2) * (1 + 3 * 2));
  CHECK(1, -2 + 3);
  CHECK(1, -2 + 3);
  CHECK(6, +2 + +2 - -2);

  CHECK(1, 1 == 1);
  CHECK(1, 1 <= 1);
  CHECK(1, 1 >= 1);
  CHECK(1, 1 > 0);
  CHECK(0, 1 < 0);
  CHECK(0, 0 > 1);
  CHECK(1, 0 < 1);
  CHECK(1, 1 >= 0);
  CHECK(0, 1 <= 0);
  CHECK(0, 0 >= 1);
  CHECK(1, 0 <= 1);
  CHECK(1, 0 != 1);

  CHECK(2, (1 == 1) + (0 != 1));
  CHECK(2, (3 <= 3) + (3 < 3) + (4 > 4) + (4 >= 4));
  CHECK(1, (2 * 3 > 2 * 2) + (2 * 3 <= 2 * 2));
  return 0;
}
