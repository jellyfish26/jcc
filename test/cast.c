#include "test.h"

int main() {
  CHECK(-1, (long long int)-1);
  CHECK(736033791, (int)82340412415);
  CHECK(-1025, (short)82340412415);
  CHECK(-1, (char)82340412415);
  return 0;
}
