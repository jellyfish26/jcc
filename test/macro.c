#include "test.h"

#define ONE 1
#define MACRO_INVOCATE TWO
#define TWO 2
#define MACRO_STRUCT struct A
#define HELLO "hello"
#define ARRAY_INITIALIZER {1, 2, 3, 4, 5};

#define FOO() 10

#define FUNC() func()

int func() {
  return 2;
}

int main() {
  CHECK(1, ONE);
  CHECK(2, TWO);
  CHECK(2, MACRO_INVOCATE);

  CHECK(3, ({
    MACRO_STRUCT {
      int a;
    };
    MACRO_STRUCT tmp = {3};
    tmp.a;
  }));

  CHECK(3, ({
    typedef MACRO_STRUCT A;
    MACRO_STRUCT {
      int a;
    };
    A tmp = {3};
    tmp.a;
  }));

#define THREE 3
  CHECK(3, THREE);

  CHECK(108, ({
    char *str = HELLO;
    str[2];
  }));

  CHECK(15, ({
    int a[5] = ARRAY_INITIALIZER
    a[0] + a[1] + a[2] + a[3] + a[4];
  }));

  CHECK(15, ({
    int a[5] = ARRAY_INITIALIZER
    a[0] + a[1] + a[2] + a[3] + a[4];
  }));

  CHECK(19, ({
    int FOO = 9;
    int BAR = FOO();
    FOO + BAR;
  }));

  CHECK(2, FUNC());

  return 0;
}
