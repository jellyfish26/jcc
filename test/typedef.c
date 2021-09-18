#include "test.h"

typedef int dogint;
typedef long catlong;

typedef struct {
  int a, b;
} tydef_st;

typedef struct DEEP DEEP;
struct DEEP {
  DEEP *deep;
  int a;
};

int main() {
  CHECK(2, ({
    dogint dog = 2;
    dog;
  }));

  CHECK(4, ({
    sizeof(dogint);
  }));

  CHECK(3, ({
    catlong cat = 3;
    cat;
  }));

  CHECK(8, ({
    sizeof(catlong);
  }));

  CHECK(10, ({
    tydef_st foo = {2, 8};
    foo.a + foo.b;
  }));

  CHECK(5, ({
    typedef struct {
      float a, b;
    } foo, bar;
    foo hoge = {1.0f, 2.0f};
    bar fuga = {2.0f, 3.0f};
    hoge = fuga;
    hoge.a + fuga.b;
  }));

  CHECK(5, ({
    typedef struct A {
      float a, b;
    } foo;
    foo hoge = {1.0f, 2.0f};
    struct A fuga = {2.0f, 3.0f};
    hoge = fuga;
    hoge.a + fuga.b;
  }));

  CHECK(3, ({
    typedef enum {
      AA,
      BB,
      CC,
      DD,
    } hoge;
    hoge aa = DD;
    aa;
  }));

  CHECK(3, ({
    DEEP foo = {};
    foo.a = 1;
    DEEP bar = {};
    bar.deep = &foo;
    bar.a = 2;
    bar.a + bar.deep->a;
  }));

  return 0;
}
