void check(int expected, int actual, char *str);
#define CHECK(expected, actual) check(expected, actual, #actual)

#define ONE 1
#define MACRO_INVOCATE TWO
#define TWO 2
#define MACRO_STRUCT struct A
#define HELLO "hello"
#define ARRAY_INITIALIZER {1, 2, 3, 4, 5};

#define FOO() 10

#define FUNC() func()
#define MAX(a, b) a > b ? a : b
#define MIN(a, b) (MAX(a, b)) == a ? b : a

int func() {
  return 2;
}

int cat(int a) {
  return a;
}

#define xstr(s) str(s)
#define str(s) #s

int main() {
  check(1, ONE, "ONE");
  check(2, TWO, "TWO");
  check(2, MACRO_INVOCATE, "MACRO_INVOCATE");

  check(3, ({
    MACRO_STRUCT {
      int a;
    };
    MACRO_STRUCT tmp = {3};
    tmp.a;
  }), "MACRO_STRUCT_1");

  check(3, ({
    typedef MACRO_STRUCT A;
    MACRO_STRUCT {
      int a;
    };
    A tmp = {3};
    tmp.a;
  }), "MACRO_STRUCT_2");

#define THREE 3
  check(3, THREE, "THREE");

  check(108, ({
    char *str = HELLO;
    str[2];
  }), "HEELO");

  check(15, ({
    int a[5] = ARRAY_INITIALIZER
    a[0] + a[1] + a[2] + a[3] + a[4];
  }), "ARRAY_INITIALIZER_1");

  check(15, ({
    int a[5] = ARRAY_INITIALIZER
    a[0] + a[1] + a[2] + a[3] + a[4];
  }), "ARRAY_INITIALIZER_2");

  check(19, ({
    int FOO = 9;
    int BAR = FOO();
    FOO + BAR;
  }), "FOO");

  check(2, FUNC(), "FUNC()");
  check(3, MAX(2, 3), "MAX(2, 3)");
  check(2, MIN(2, 3), "MIN(2, 3)");
  check(2, MIN(func(), 3), "MIN(func(), 3)");
  check(2, MIN(FUNC(), 3), "MIN(FUNC(), 3)");
  check(2, MIN(cat(func()), 3), "MIN(cat(func()), 3)");
  check(2, MIN(cat(FUNC()), 3), "MIN(cat(FUNC()), 3)");

  // stringizing
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
  CHECK(3, MAX(2, 3));
  CHECK(2, MIN(2, 3));
  CHECK(2, MIN(func(), 3));
  CHECK(2, MIN(FUNC(), 3));
  CHECK(2, MIN(cat(func()), 3));
  CHECK(2, MIN(cat(FUNC()), 3));

  CHECK(51, ({
    char *str = xstr(THREE);
    *str;
  }));

  CHECK(84, ({
    char *str = str(THREE);
    *str;
  }));

  return 0;
}
