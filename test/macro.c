void check(int expected, int actual, char *str);
void checkl(long expected, long actual, char *str);
void checkul(unsigned long expected, unsigned long actual, char *str);
void checkf(float expected, float actual, char *str);
void checkd(double expected, double actual, char *str);
void checkld(long double expected, long double actual, char *str);

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

  // check(2, FUNC(), "FUNC()");

  return 0;
}
