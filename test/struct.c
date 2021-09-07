#include "test.h"

struct A {
  int A;
  int B;
};

int main() {
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

  return 0;
}
