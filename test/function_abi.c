#include "test.h"
#include "function_abi.h"

int main() {
  CHECK(5, ({int a = 2, b = 3; add2(a, b);}));
  CHECK(1, ({int a = 2; int b = 3; sub2(b, a);}));
  CHECK(6, ({int a = 5, b = 3; add2(a - b, b + 1);}));
  CHECK(5, ({int a = 5, b = 2; add2(sub2(a, b), b);}));
  CHECK(21, ({
    int a = 1; int b = 2; int c = 3; int d = 4; int e = 5; int f = 6;
    add6(a, b, c, d, e, f);
  }));
  CHECK(36, ({
    int a = 1; int b = 2; int c = 3; int d = 4; int e = 5; int f = 6; int g = 7; int h = 8;
    add8(a, b, c, d, e, f, g, h);
  }));
  CHECK(55, ({fib(10);}));

  CHECK(2, vv());
  CHECK(3, foo(2, 1));
  CHECK(3, da(2.0, 1));
  CHECK(6, dla(5.0l, 1));

  CHECK(66, dlb(1.0l, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11));

  CHECK(6, ({
    struct A tmp = {2, 3, 1};
    sa(tmp);
  }));

  CHECK(2, ({
    struct A tmp = {4, 3, 1};
    int ans = tmp.a + tmp.b + tmp.c;
    ans - sb(tmp);
  }));

  CHECK(21, ({
    struct B tmp = {{1, 2, 3, 4, 5, 6}};
    sc(tmp);
  }));

  CHECKLD(3.0, ({
    struct C tmp = {3.0};
    sd(tmp);
  }));

  CHECK(15, ({
    int tmp[5] = {1, 2, 3, 4, 5};
    arr1(tmp);
  }));

  CHECK(0, ({
    int tmp[] = {1, 2, 3, 4, 5};
    int ans = arr2(tmp);
    for (int i = 0; i < 5; i++) {
      ans -= tmp[i];
    }
    ans;
  }));

  CHECKF(5.0, ({
    struct D tmp = {2, 3.0f};
    se(tmp);
  }));

  CHECKLD(4.1l, ({
    struct E tmp = {1, 3.1l};
    sf(tmp);
  }));

  CHECKLD(9.1l, ({
    struct E tmp = {1, 3.1l};
    sg(tmp, 2.0l, 3);
  }));

  CHECKF(13.0f, ({
    struct D tmp = {2, 3.0f};
    si(tmp, 2, 3, 3.0f);
  }));

  CHECK(48, ({
    struct F tmp;
    tmp.a = 2;
    tmp.b = 50;
    sj(tmp);
  }));

  CHECK(288, ({
    struct F tmp;
    tmp.a = 2;
    tmp.b = 50;
    tmp.c = 1;
    tmp.d = -1;
    tmp.e = 240;
    sk(tmp);
  }));

  CHECK(14, ({
    union G tmp;
    tmp.b = 11;
    ua(tmp);
  }));

  CHECK(10, ({
    union G tmp;
    tmp.b = 11;
    tmp.a = -3;
    ua(tmp);
  }));

  CHECK(6, ({
    struct A tmp;
    tmp = retsta();
    tmp.a + tmp.b + tmp.c;
  }));

  CHECK(21, ({
    struct B tmp;
    tmp = retstb();
    int ans = 0;
    for (int i = 0; i < 6; i++) {
      ans += tmp.a[i];
    }
    ans;
  }));

  CHECKLD(2.2L, ({
    struct C tmp;
    tmp = retstc();
    tmp.a;
  }));

  CHECKF(5.0f, ({
    struct D tmp;
    tmp = retstd();
    tmp.a + tmp.b;
  }));

  CHECKLD(5.0l, ({
    struct E tmp;
    tmp = retste();
    tmp.a + tmp.b;
  }));

  CHECK(288, ({
    struct F tmp;
    tmp = retstf();
    tmp.a + tmp.b + tmp.c + tmp.d + tmp.e;
  }));

  return 0;
}

