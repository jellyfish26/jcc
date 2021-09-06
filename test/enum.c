#include "test.h"

enum A {
  hoge,
  fuga,
};

enum B {
  foo = 1LL<<32,
  bar,
  potato = 1,
  tomato,
};

int main() {
  CHECK(0, hoge);
  CHECK(1, fuga);

  CHECKL(4294967296, foo);
  CHECKL(4294967297, bar);
  CHECKL(1, potato);
  CHECKL(2, tomato);

  CHECKL(1, ({
    enum B tmp = potato;
    tmp;
  }));

  CHECK(0, ({
    enum {fuga, hoge} tmp = fuga;
    tmp;
  }));

  CHECK(2, ({
    enum A {
      potato,
      tomato,
      banana,
    };
    enum A tmp = banana;
    tmp;
  }));

  CHECK(4, sizeof(enum A));
  CHECK(8, sizeof(enum B));
  CHECK(8, ({
    enum B tmp = potato;
    sizeof(tmp);
  }));

  return 0;
}
