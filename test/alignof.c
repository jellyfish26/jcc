#include "test.h"

int main() {
  CHECK(1, _Alignof(char));
  CHECK(2, _Alignof(unsigned short));
  CHECK(4, _Alignof(int));
  CHECK(8, _Alignof(long));
  CHECK(8, _Alignof(long int));
  CHECK(8, _Alignof(long long));
  CHECK(1, _Alignof(char[3]));
  CHECK(16, _Alignof(long double[10]));
  CHECK(1, _Alignof(struct {char a; char b;}));
  CHECK(4, _Alignof(struct {char a; int b;}[2]));

  CHECK(8, ({
    struct {
      char c;
      long b;
    } tmp;
    _Alignof(tmp);
  }));

  return 0;
}
