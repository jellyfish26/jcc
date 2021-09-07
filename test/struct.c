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

  return 0;
}
