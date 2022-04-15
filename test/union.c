#include "test.h"

union A {
  int A;
  int B;
};

union A g = {2};

union C {
  int a: 3;
  int b: 8;
};

union C ca = {-3};
union C cb = {.b = 11};

int main() {
  CHECK(4, sizeof(union A));

  CHECK(8, ({
    union B {
      int a;
      long b;
    };
    sizeof(union B);
  }));

  CHECK(2, g.A);
  CHECK(2, g.B);

  CHECK(2, ({
    union A tmp;
    tmp.A = 1;
    tmp.B = 2;
    tmp.B;
  }));

  CHECK(5, ({
    struct B {
      union A aa;
      int bb;
    };
    struct B tmp;
    tmp.aa.A = 1;
    tmp.aa.B = 2;
    tmp.bb = 3;
    tmp.bb + tmp.aa.B;
  }));

  CHECK(2, ({
    union A tmp;
    tmp.A = 1;
    tmp.B = 2;
    union A *ptr = &tmp;
    ptr->A;
  }));

  CHECK(5, ({
    struct B {
      union A aa;
      int bb;
    };
    struct B tmp;
    tmp.aa.A = 1;
    tmp.aa.B = 2;
    tmp.bb = 3;

    struct B *ptr = &tmp;
    ptr->aa.A + ptr->bb;
  }));

  CHECK(5, ({
    struct B {
      union A *aa;
      int bb;
    };
    struct B tmp;
    union A hello;
    hello.A = 1;
    hello.B = 2;
    tmp.aa = &hello;
    tmp.bb = 3;

    tmp.bb + tmp.aa->B;
  }));

  CHECK(6, ({
    struct B {
      union A aa;
      int bb;
    };
    struct B tmp;
    tmp.aa.A = 1;
    tmp.aa.B = 2;
    tmp.bb = 3;

    struct B *ptr = &tmp;
    ptr->bb++;
    ptr->aa.B + ptr->bb;
  }));

  CHECK(5, ({
    union B {
      int b;
      int hoge[5];
    };
    union B tmp;
    tmp.hoge[0] = 1;
    tmp.hoge[3] = 2;
    tmp.b = 3;

    union B *cat = &tmp;
    cat->hoge[0] + tmp.hoge[3];
  }));

  CHECK(5, ({
    struct B {
      union A aa;
      int bb;
    };
    struct B tmp = {
      {1},
      3
    };
    tmp.aa.B + tmp.aa.A + tmp.bb;
  }));

  CHECK(7, ({
    union B {
      int aa[3];
      int bb;
    };
    union B tmp = {
      {1, 2, 3},
    };
    tmp.aa[0] + tmp.aa[1] + tmp.aa[2] + tmp.bb;
  }));

  CHECK(8, ({
    union B {
      int aa[3];
      int bb;
    };
    union B foo = {
      {1, 2, 3},
    };
    union B bar;
    bar = foo;
    bar.bb = 5;
    bar.bb + foo.bb + foo.aa[1];
  }));

  CHECK(12, ({
    union A foo;
    foo.A = 1;
    foo.B = 2;
    union A bar;
    bar = foo;
    bar.B = 3;
    union A hoge = foo = bar;
    hoge.B + foo.B + bar.B + hoge.A;
  }));

  CHECK(8, ({
    union A foo;
    foo.A = 1;
    foo.B = 2;
    union A *bar = &foo;
    union A hoge;
    union A *fuga = &hoge;
    *fuga = *bar;
    fuga->A + fuga->B + hoge.A + hoge.B;
  }));

  CHECK(14, ({
    union B {
      int a: 3;
      int b: 8;
    };
    union B tmp;
    tmp.b = 11;
    tmp.a + tmp.b;
  }));

  CHECK(10, ({
    union B {
      int a: 3;
      int b: 8;
    };
    union B tmp;
    tmp.b = 11;
    tmp.a = -3;
    tmp.a + tmp.b;
  }));

  CHECK(2, ({
    union B {
      int a: 3;
      int b: 8;
    };
    union B tmp = {-3};
    tmp.a + tmp.b;
  }));

  CHECK(11, ({
    union B {
      int a: 3;
      int b: 8;
    };
    union B tmp = {.b = 11};
    tmp.b;
  }));

  CHECK(10, ({
    union B {
      int a: 3;
      int b: 8;
    };
    union B tmp = {.b = 11};
    tmp.a = -3;
    tmp.a + tmp.b;
  }));

  CHECK(-3, ca.a);
  CHECK(5, ca.b);
  CHECK(11, cb.b);
  CHECK(14, cb.a + cb.b);

  return 0;
}
