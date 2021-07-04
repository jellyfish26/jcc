#include "test.h"

int main() {
  CHECK(2, ({int hoge = 3; if (hoge == 3) hoge = 2; hoge; }));
  CHECK(3,({if (2 == 3) 2; 3; }));
  CHECK(4,({int hoge = 2; int fuga = 4; if (hoge == fuga) hoge; fuga; }));
  CHECK(5,({int foo = 2; int bar = 3; if (foo < bar) foo = foo + bar; foo; }));
  CHECK(2,({int foo = 2; int bar = 3; if (foo > bar) foo = foo + bar; foo; }));

  CHECK(2,({int foo = 0; if (foo == 0) foo = 2; else foo = 3; foo; }));
  CHECK(3,({int foo = 0; if (foo != 0) foo = 2; else foo = 3; foo; }));

  CHECK(4,({int hoge = 2; int fuga = 3; if (hoge == 2) { fuga = 2; hoge = hoge * fuga; } else { fuga = 5; hoge = fuga - hoge; } hoge; }));
  CHECK(3,({int hoge = 2; int fuga = 3; if (hoge == 1) { fuga = 2; hoge = hoge * fuga; } else { fuga = 5; hoge = fuga - hoge; } hoge; }));

  CHECK(2,({int hoge = 3; if (hoge == 2) hoge = 1; else if (hoge == 3) hoge = 2; else hoge = 3; hoge; }));

  CHECK(20,({int i; int hoge = 0; for (i = 0; i < 10; i = i + 1) { hoge = hoge + 2; } hoge; }));
  CHECK(20,({int i; int hoge = 0; for (i = 0; i < 10; i = i + 1) hoge = hoge + 2; hoge; }));
  CHECK(2 ,({int i = 10; int hoge = 22; for (; i > 0; i = i - 1) { hoge = hoge - 2; } hoge; }));
  CHECK(10,({int hoge = 0; for (int i = 0; i < 10;) { hoge = hoge + 1; i = i + 1; } hoge; }));
  CHECK(55,({int hoge = 0; for (int i = 1; i <= 10; i = i + 1) { hoge = hoge + i; } hoge; }));
  CHECK(25,({int hoge = 0; for (int i = 0; i < 5; i = i + 1) { for (int j = 0; j < 5; j = j + 1) { hoge = hoge + 1; }} hoge; }));

  CHECK(6 ,({int i = 0; int j; while (i < 5) i = i + 2; i; }));
  CHECK(10,({int i = 4; int hoge = 0; while (i >= 0) { i = i - 1; hoge = hoge + 2; } hoge; }));

  CHECK(3,({int hoge = 0; while (1) { hoge = hoge + 1; if (hoge == 3) break; } hoge; }));
  CHECK(8,({int hoge = 0; while (1) { hoge = hoge + 1; if (hoge <= 5) continue; hoge = hoge + 2; break; } hoge; }));
  CHECK(6,({int hoge = 0; for (int i = 0; i < 5; i = i + 1) { hoge = hoge + 2; if (i >= 2) break; } hoge; }));
  CHECK(100, ({
    int ans = 0;
    for (int i = 0; i < 5; ++i) {
      for (int j = 0; j < 5; ++j) {
        ans = ans + i + j;
      }
    }
    ans;
  }));

  CHECK(2, ({
    int a = 1;
    int b = ++a;
    b;
  }));

  CHECK(1, ({
    int a = 10;
    int b = 3;
    int c = a % b;
    c;
  }));

  CHECK(10, ({
    long a = 162;
    long b = 38;
    long c = a % b;
    c;
  }));

  CHECK(32, ({
    int a = 2;
    int b = 1<<2;
    int c = 0;
    c = (a<<b);
    c;
  }));

  CHECK(16, ({
    int a = 1<<10;
    int b = 6;
    int c = a>>b;
    c;
  }));

  CHECK(10, ({
    int a = 15;
    int b = 26;
    a & b;
  }));

  CHECK(11, ({
    int a = 22;
    int b = 29;
    int c = a ^ b;
    c;
  }));

  CHECK(61, ({
    int a = 41;
    int b = 28;
    a | b;
  }));

  CHECK(5, ({
    int ans = 0;
    int a = 2;
    int b = 3;
    if (a == 2 && b == 3) {
      ans = 5;
    }
    ans;
  }));

  CHECK(3, ({
    int ans = 0;
    int a = 2;
    int b = 3;
    if (a + b == 5 && a == 3) {
      ans = 5;
    } else {
      ans = 3;
    }
    ans;
  }));

  CHECK(10, ({
    int ans = 0;
    int a = 2;
    int b = 3;
    if (a == 3 || a + b == 5 || b == 2) {
      ans = 10;
    }
    ans;
  }));

  CHECK(2, ({
    int ans = 0;
    int a = 3;
    int b = 2;
    if (a == b || a * b == a || (a | b) == 2) {
      ans = 5;
    } else {
      ans = 2;
    }
    ans;
  }));

  CHECK(3, ({
    int ans = 0;
    int a = 2;
    int b = 1;
    ans = (a - b == 1 ? 3 : 2);
    ans;
  }));

  CHECK(5, ({
    int ans = 0;
    int a = 3;
    int b = 5;
    ans = (a + b == 7 ? ans == 0 ? 1 : 2 : a + b == 8 ? b : a);
    ans;
  }));

  CHECK(12, ({
    int a = 2;
    int b = 4;
    a = b = 6;
    a + b;
  }));

  CHECK(10, ({
    int a = 2;
    a += 5;
    a + 3;
  }));

  CHECK(18, ({
    int a = 2;
    int b = 3;
    a += b += 5;
    a + b;
  }));

  CHECK(5, ({
    int a = 10;
    a -= 7;
    a + 2;
  }));

  CHECK(10, ({
    int a = 10;
    int b = 3;
    int c = 7;
    a -= c -= b;
    a + c;
  }));

  CHECK(9, ({
    int a = 3;
    int b = 5;
    a += b -= 2;
    a + b;
  }));

  CHECK(15, ({
    int a = 2;
    int b = 3;
    a *= b += 2;
    a + b;
  }));

  CHECK(18, ({
    int a = 3;
    int b = 2;
    a *= b *= 3;
    a;
  }));

  CHECK(3, ({
    int a = 6;
    int b = 4;
    a /= b /= 2;
    a;
  }));

  CHECK(10, ({
    long a = 30;
    long b = 6;
    a /= b /= 2;
    a;
  }));

  CHECK(10, ({
    int a = 3;
    int b = 5;
    a %= b %= 3;
    a + 9;
  }));

  CHECK(32, ({
    int a = 2;
    a <<= 4;
    a;
  }));

  CHECK(4, ({
    int a = 128;
    a >>= 5;
    a;
  }));

  CHECK(20, ({
    int a = 2;
    int b = 1;
    b <<= a <<= 1;
    b + a;
  }));

  CHECK(5, ({
    int a = 5;
    int b = 15;
    a &= b &= 29;
    a;
  }));

  CHECK(23, ({
    int a = 5;
    int b = 15;
    a ^= b ^= 29;
    a;
  }));

  CHECK(31, ({
    int a = 5;
    int b = 15;
    a |= b |= 29;
    a;
  }));

  CHECK(9, ({
    int a = 4;
    int b = a++;
    a + b;
  }));

  CHECK(9, ({
    int a = 4;
    int b = a++ + a++;
    b;
  }));

  CHECK(10, ({
    int a = 5;
    int b = 4;
    if (!(a == b)) {
      b = 5;
    }
    a + b;
  }));

  CHECK(9, ({
    int a = 5;
    int b = 4;
    if (!(a != b)) {
      b = 5;
    }
    a + b;
  }));

  CHECK(30, ({
    int a = 30;
    int b = a & (~a);
    a + b;
  }));

  CHECK(10, ({
    int a = 2;
    int b = 5;
    {
      int b = 3;
      a += b;
    }
    a + b;
  }));
  return 0;
}
