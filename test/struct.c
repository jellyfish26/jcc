#include "test.h"

struct A {
  int A;
  int B;
};

struct A g = {2, 3};

int main() {
  CHECK(8, sizeof(struct A));

  CHECK(3, g.B);

  CHECK(2, ({
    struct A tmp;
    tmp.A = 1;
    tmp.B = 2;
    tmp.B;
  }));

  CHECK(5, ({
    struct B {
      struct A aa;
      int bb;
    };
    struct B tmp;
    tmp.aa.A = 1;
    tmp.aa.B = 2;
    tmp.bb = 3;
    tmp.bb + tmp.aa.B;
  }));

  CHECK(1, ({
    struct A tmp;
    tmp.A = 1;
    tmp.B = 2;
    struct A *ptr = &tmp;
    ptr->A;
  }));

  CHECK(5, ({
    struct B {
      struct A aa;
      int bb;
    };
    struct B tmp;
    tmp.aa.A = 1;
    tmp.aa.B = 2;
    tmp.bb = 3;

    struct B *ptr = &tmp;
    ptr->aa.B + ptr->bb;
  }));

  CHECK(5, ({
    struct B {
      struct A *aa;
      int bb;
    };
    struct B tmp;
    struct A hello;
    hello.A = 1;
    hello.B = 2;
    tmp.aa = &hello;
    tmp.bb = 3;

    tmp.bb + tmp.aa->B;
  }));

  CHECK(6, ({
    struct B {
      struct A aa;
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

  CHECK(3, ({
    struct B {
      int hoge[5];
    };
    struct B tmp;
    tmp.hoge[0] = 1;
    tmp.hoge[3] = 2;

    struct B *cat = &tmp;
    cat->hoge[0] + tmp.hoge[3];
  }));

  CHECK(6, ({
    struct B {
      struct A aa;
      int bb;
    };
    struct B tmp = {
      {1, 2},
      3
    };
    tmp.aa.B + tmp.aa.A + tmp.bb;
  }));

  CHECK(9, ({
    struct B {
      int aa[3];
      int bb;
    };
    struct B tmp = {
      {1, 2, 3},
      3,
    };
    tmp.aa[0] + tmp.aa[1] + tmp.aa[2] + tmp.bb;
  }));

  // Excess elements
  CHECK(9, ({
    struct B {
      int aa[3];
      int bb;
    };
    struct B tmp = {
      {1, 2, 3, 4},
      3, 4
    };
    tmp.aa[0] + tmp.aa[1] + tmp.aa[2] + tmp.bb;
  }));

  CHECK(6, ({
    struct A foo;
    foo.A = 1;
    foo.B = 2;
    struct A bar;
    bar = foo;
    bar.B = 3;
    bar.A + bar.B + foo.B;
  }));

  CHECK(6, ({
    struct B {
      struct A aa;
      int bb;
    };
    struct B cat = {
      {1, 2},
      3
    };
    struct B dolphin;
    dolphin = cat;
    dolphin.bb = 2;
    dolphin.bb + cat.bb + dolphin.aa.A;
  }));

  return 0;
}
