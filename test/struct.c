#include "test.h"

struct A {
  int A;
  int B;
};

struct A g = {2, 3};
struct C {
  struct A *aa;
  int B;
};

struct C gp = {&g, 2};
struct LD {
  char a;
  long double b;
};

struct LD ldg = {1, 3.1l};

struct E {
  unsigned char a: 2;
  int b: 30;
  char c: 2, d: 2;
  short e: 12;
};

struct E ea = {2, 50, 1, -1, 240};

struct F {
  int a: 2;
  int b: 10;
  unsigned int c: 5;
  int d;
};

struct F fa = {2, 100, 16, -100};

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

  CHECK(10, ({
    struct A foo;
    foo.A = 1;
    foo.B = 2;
    struct A bar;
    bar = foo;
    bar.B = 3;
    struct A hoge = foo = bar;
    hoge.B + foo.B + bar.B + hoge.A;
  }));

  CHECK(6, ({
    struct A foo;
    foo.A = 1;
    foo.B = 2;
    struct A *bar = &foo;
    struct A hoge;
    struct A *fuga = &hoge;
    *fuga = *bar;
    fuga->A + fuga->B + hoge.A + hoge.B;
  }));

  CHECK(4, gp.aa->A + gp.B);
  
  CHECK(8, ({
    struct D {
      char a, b;
      int c;
    };
    sizeof(struct D);
  }));

  CHECK(8, ({
    struct D {
      char a, b, c;
      int d;
    };
    sizeof(struct D);
  }));

  CHECK(16, ({
    struct D {
      char a, b;
      long c;
    };
    sizeof(struct D);
  }));

  CHECK(32, ({
    struct D {
      char a;
      long double b;
    };
    sizeof(struct D);
  }));

  CHECK(12, ({
    struct D {
      char a;
      int b;
      char c, d;
      short e;
    };
    sizeof(struct D);
  }));

  CHECK(16, ({
    struct D {
      char a;
      int b;
      char c, d;
      short e;
    };

    struct E {
      char a;
      struct D b;
    };
    sizeof(struct E);
  }));

  CHECKL(1, ({
    struct LD tmp = {1, 3.1l};
    tmp.a;
  }));

  CHECKLD(3.1l, ({
    struct LD tmp = {1, 3.1l};
    tmp.b;
  }));

  CHECKLD(4.1l, ({
    struct LD tmp = {1, 3.1l};
    tmp.a + tmp.b;
  }));

  CHECK(1, ldg.a);
  CHECKLD(3.1l, ldg.b);
  CHECKLD(4.1l, ldg.a + ldg.b);

  CHECK(16, ({
    struct D {
      char a;
      int b;
      char c, d;
      short e;
    };

    struct D tmp = {.b = 2, 6, .a = 3, .e = 5};
    tmp.a + tmp.b + tmp.c + tmp.d + tmp.e;
  }));

  CHECK(6, ({
    struct D {
      char a;
      int b;
      char c, d;
      short e;
    };

    struct D tmp = {.b = 2, 6, .a = 3, .e = 5};
    tmp.d + tmp.c;
  }));

  CHECK(6, ({
    struct D {
      char a: 2;
      int b: 30;
      char c: 2, d: 2;
      short e: 12;
    };
    sizeof(struct D);
  }));

  CHECK(-2, ({
    struct D {
      char a: 2;
      int b: 30;
      char c: 2, d: 2;
      short e: 12;
    };
    struct D tmp;
    tmp.a = 2;
    tmp.a;
  }));

  CHECK(48, ({
    struct D {
      char a: 2;
      int b: 30;
      char c: 2, d: 2;
      short e: 12;
    };
    struct D tmp;
    tmp.a = 2;
    tmp.b = 50;
    tmp.b + tmp.a;
  }));

  CHECK(288, ({
    struct D {
      char a: 2;
      int b: 30;
      char c: 2, d: 2;
      short e: 12;
    };
    struct D tmp;
    tmp.a = 2;
    tmp.b = 50;
    tmp.c = 1;
    tmp.d = -1;
    tmp.e = 240;
    tmp.a + tmp.b + tmp.c + tmp.d + tmp.e;
  }));

  CHECK(292, ({
    struct D {
      unsigned char a: 2;
      int b: 30;
      char c: 2, d: 2;
      short e: 12;
    };
    struct D tmp = {2, 50, 1, -1, 240};
    tmp.a + tmp.b + tmp.c + tmp.d + tmp.e;
  }));

  CHECK(293, ({
    struct D {
      unsigned char a: 2;
      int b: 30;
      char c: 2, d: 2;
      short e: 12;
    };
    struct D tmp = {.b = 50, 1, .e = 240, .a = 2};
    tmp.a + tmp.b + tmp.c + tmp.d + tmp.e;
  }));

  CHECK(292, ea.a + ea.b + ea.c + ea.d + ea.e);

  CHECK(8, sizeof(struct F));
  CHECK(14, fa.a + fa.b + fa.c + fa.d);
  CHECK(14, ({
    struct F tmp = {2, 100, 16, -100};
    tmp.a + tmp.b + tmp.c + tmp.d;
  }));

  CHECK(6, ({
    struct B {
      struct A aa;
      int bb;
    };
    struct A dog = {1, 2};
    struct B tmp = {dog, 3};
    tmp.aa.A + tmp.aa.B + tmp.bb;
  }));

  return 0;
}
