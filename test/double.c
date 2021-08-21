#include "test.h"

int main() {
  CHECK(2.3, 2.3);
  CHECK(5.3, 2.3 + 2.0);

  return 0;
}
