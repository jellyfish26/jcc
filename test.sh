gcc -std=c11 -static -c -o ./test/print.o ./test/print.c

assert() {
  expected="$1"
  input="$2"

  echo "$input" > tmp.c

  ./jcc tmp.c > tmp.s
  gcc -static -o tmp tmp.s ./test/print.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo -e "\e[32m[ OK ]\e[m $input => $actual"
  else
    echo -e "\e[31m[ NG ]\e[m $input => $expected expected, but got $actual"
    exit 1
  fi
}

stdio_test() {
  expected="$1"
  input="$2"

  echo "$input" > tmp.c

  ./jcc tmp.c > tmp.s
  gcc -static -o tmp tmp.s ./test/print.o
  
  actual=$(./tmp)
  
  if [ "$actual" = "$expected" ]; then
    echo -e "\e[32m[ OK ]\e[m $input => $actual"
  else
    echo -e "\e[31m[ NG ]\e[m $input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 5  "int main() { return 2 + 3; }"
assert 3  "int main() { return 5 - 2; }"
assert 4  "int main() { return 2 * 2; }"
assert 4  "int main() { return 8 / 2; }"
assert 5  "int main() { return 2 * 2 + 1 * 1; }"
assert 10 "int main() { return 2 * 2 / 2 + 8 / 2 + 6 - 2; }"
assert 5  "int main() { return (2 + 3) * (3 - 2); }"
assert 14 "int main() { return ((2 * 3 - 2) / 2) * (1 + 3 * 2); }"

assert 1 "int main() { return -2 + 3; }"
assert 6 "int main() { return +2 + +2 - -2; }"

assert $((1 == 1)) "int main() { return 1 == 1; }"
assert $((1 <= 1)) "int main() { return 1 <= 1; }"
assert $((1 >= 1)) "int main() { return 1 >= 1; }"
assert $((1 > 0))  "int main() { return 1 > 0; }"
assert $((1 < 0))  "int main() { return 1 < 0; }"
assert $((0 > 1))  "int main() { return 0 > 1; }"
assert $((0 < 1))  "int main() { return 0 < 1; }"
assert $((1 >= 0)) "int main() { return 1 >= 0; }"
assert $((1 <= 0)) "int main() { return 1 <= 0; }"
assert $((0 >= 1)) "int main() { return 0 >= 1; }"
assert $((0 <= 1)) "int main() { return 0 <= 1; }"
assert $((0 != 1)) "int main() { return 0 != 1; }"

assert 2 "int main() { return (1 == 1) + (0 != 1); }"
assert 2 "int main() { return (3 <= 3) + (3 < 3) + (4 > 4) + (4 >= 4); }"
assert 1 "int main() { return (2 * 3 > 2 * 2) + (2 * 3 <= 2 * 2); }"

assert 2 "int main() { int hoge; hoge = 2; return hoge; }"
assert 3 "int main() { int test = 2; test = 3; return test; }"
assert 6 "int main() { int foo = 2; int bar = 3; return foo * bar; }"

assert 2 "int main() { int hoge = 3; if (hoge == 3) hoge = 2; return hoge; }"
assert 3 "int main() { if (2 == 3) return 2; return 3; }"
assert 4 "int main() { int hoge = 2; int fuga = 4; if (hoge == fuga) return hoge; return fuga; }"
assert 5 "int main() { int foo = 2; int bar = 3; if (foo < bar) foo = foo + bar; return foo; }"
assert 2 "int main() { int foo = 2; int bar = 3; if (foo > bar) foo = foo + bar; return foo; }"

assert 2 "int main() { int foo = 0; if (foo == 0) foo = 2; else foo = 3; return foo; }"
assert 3 "int main() { int foo = 0; if (foo != 0) foo = 2; else foo = 3; return foo; }"

assert 4 "int main() { int hoge = 2; int fuga = 3; if (hoge == 2) { fuga = 2; hoge = hoge * fuga; } else { fuga = 5; hoge = fuga - hoge; } return hoge; }"
assert 3 "int main() { int hoge = 2; int fuga = 3; if (hoge == 1) { fuga = 2; hoge = hoge * fuga; } else { fuga = 5; hoge = fuga - hoge; } return hoge; }"

assert 2 "int main() { int hoge = 3; if (hoge == 2) hoge = 1; else if (hoge == 3) hoge = 2; else hoge = 3; return hoge; }"

assert 20 "int main() { int i; int hoge = 0; for (i = 0; i < 10; i = i + 1) { hoge = hoge + 2; } return hoge; }"
assert 20 "int main() { int i; int hoge = 0; for (i = 0; i < 10; i = i + 1) hoge = hoge + 2; return hoge; }"
assert 2  "int main() { int i = 10; int hoge = 22; for (; i > 0; i = i - 1) { hoge = hoge - 2; } return hoge; }"
assert 10 "int main() { int hoge = 0; for (int i = 0; i < 10;) { hoge = hoge + 1; i = i + 1; } return hoge; }"
assert 55 "int main() { int hoge = 0; for (int i = 1; i <= 10; i = i + 1) { hoge = hoge + i; } return hoge; }"
assert 25 "int main() { int hoge = 0; for (int i = 0; i < 5; i = i + 1) { for (int j = 0; j < 5; j = j + 1) { hoge = hoge + 1; }} return hoge; }"

assert 6  "int main() { int i = 0; int j; while (i < 5) i = i + 2; return i; }"
assert 10 "int main() { int i = 4; int hoge = 0; while (i >= 0) { i = i - 1; hoge = hoge + 2; } return hoge; }"

assert 3 "int main() { int hoge = 0; while (1) { hoge = hoge + 1; if (hoge == 3) break; } return hoge; }"
assert 8 "int main() { int hoge = 0; while (1) { hoge = hoge + 1; if (hoge <= 5) continue; hoge = hoge + 2; break; } return hoge; }"
assert 6 "int main() { int hoge = 0; for (int i = 0; i < 5; i = i + 1) { hoge = hoge + 2; if (i >= 2) break; } return hoge; }"

stdio_test "foo" "int main() { foo(); return 0; }"
stdio_test "1, 2, 3, -1, 2, 0" "int main() { int i = 1; int j = 2; hoge(i, j, i + j, i - j, i * j, i / j); return 0; }"

stdio_test "foo" "int fuga() { foo(); } int main() { fuga(); return 0; }"

stdio_test "3, -1, 2, 4, -3, -3" "
int test(int a, int b) {
  hoge(a, b, a + b, a - b, a * b, a / b); 
}

int main() {
  int i = 1; int j = 2;
  test(i + j, i - j);
}
"

stdio_test "6, 5, 4, 3, 2, 1" "
int test(int a, int b, int c, int d, int e, int f) {
  hoge(f, e, d, c, b, a); 
}

int main() {
  int a = 1; int b = 2; int c = 3; int d = 4; int e = 5; int f = 6;
  test(a, b, c, d, e, f);
}
"

stdio_test "6, 5, 4, 3, 2, 1" "
int test(int a, int b, int c, int d, int e, int f) {
  int g = 3;
  hoge(f, e, d, c, b, a); 
}

int main() {
  int a = 1; int b = 2; int c = 3; int d = 4; int e = 5; int f = 6;
  test(a, b, c, d, e, f);
}
"

stdio_test "1, 2, 3, 4, 5, 6, 7, 8" "
int main() {
  int a = 1; int b = 2; int c = 3; int d = 4; int e = 5; int f = 6; int g = 7; int h = 8;
  eight_print(a, b, c, d, e, f, g, h);
}
"

stdio_test "8, 7, 6, 5, 4, 3, 2, 1" "
int test(int a, int b, int c, int d, int e, int f, int g, int h) {
  eight_print(h, g, f, e, d, c, b, a); 
}

int main() {
  int a = 1; int b = 2; int c = 3; int d = 4; int e = 5; int f = 6; int g = 7; int h = 8;
  test(a, b, c, d, e, f, g, h);
}
"

assert 55 "
int test(int a) {
  if (a == 0) {
    return 0;
  }
  return test(a - 1) + a;
}

int main() {
  int a = 10;
  return test(a);
}
"

assert 10 "
long test(long a, long b, int c) {
  a = a + c;
  return a + b;
}

int main() {
  long a = 3;
  long b = 5;
  int c = 2;
  return test(a, b, c);
}
"

assert 8 "
int main() {
  int a = 2;
  int b = 5;
  int *c = &a;
  return 1 + *(&b) + *c;
}
"

assert 5 "
int main() {
  int a = 2;
  int b = 3;
  int *c = &a;
  return b + *c;
}
"

assert 10 "
int main() {
  int a[3];
  *a = 1;
  *(a + 1) = 3;
  *(a + 2) = 6;
  int ans = 0;
  for (int i = 0; i < 3; i = i + 1) {
    ans = ans + *(a + i);
  }
  return ans;
}
"

assert 10 "
int main() {
  int a[3];
  a[0] = 1;
  a[1] = 3;
  a[2] = 6;
  int ans = 0;
  for (int i = 0; i < 3; i = i + 1) {
    ans = ans + a[i];
  }
  return ans;
}
"


assert 10 "
int main() {
  int a[3];
  a[0] = 1;
  a[1] = 3;
  a[2] = 6;
  int ans = 0;
  ans = ans + *(&a[0]);
  ans = ans + *(&(*(a + 1)));
  ans = ans + *(&(*(&(*(&a[2])))));
  return ans;
}
"

assert 4 "
int main() {
  int a[2][2];
  int ans = 0;
  hi(a[0][0]);
  for (int i = 0; i < 2; i = i + 1) {
    for (int j = 0; j < 2; j = j + 1) {
      a[i][j] = i + j;
      ans = ans + a[i][j];
    }
  }
  return ans;
}
"

assert 96 "
int main() {
  int a[2][3][4];
  int i;
  int j;
  int k;
  for (i = 0; i < 2; i = i + 1) {
    for (j = 0; j < 3; j = j + 1) {
      for (k = 0; k < 4; k = k + 1) {
        a[i][j][k] = i + j + k + 1;
      }
    }
  }
  int ans = 0;
  for (i = 0; i < 2; i = i + 1) {
    for (j = 0; j < 3; j = j + 1) {
      for (k = 0; k < 4; k = k + 1) {
        ans = ans + a[i][j][k];
      }
    }
  }
  return ans;
}
"

assert 15 "
int main() {
  int a[3][3];
  a[1][2] = 7;
  *(*(a + 2)) = 5;
  *(*(a) + 1) = 2;
  *(*(a + 2) + 2) = 1;
  int ans = *(*(a + 1) + 2);
  ans = ans + a[2][0];
  ans = ans + *(*(a) + 1);
  ans = ans + a[2][2];
  return ans;
}
"

assert 100 "
int main() {
  int ans = 0;
  for (int i = 0; i < 5; ++i) {
    for (int j = 0; j < 5; ++j) {
      ans = ans + i + j;
    }
  }
  return ans;
}
"

assert 2 "
int main() {
  int a = 1;
  int b = ++a;
  return b;
}
"

assert 1 "
int main() {
  int a = 10;
  int b = 3;
  int c = a % b;
  return c;
}
"

assert 10 "
int main() {
  long a = 162;
  long b = 38;
  long c = a % b;
  return c;
}
"

assert 32 "
int main() {
  int a = 2;
  int b = 1<<2;
  int c = 0;
  c = (a<<b);
  return c;
}
"

assert 16 "
int main() {
  int a = 1<<10;
  int b = 6;
  int c = a>>b;
  return c;
}
"

assert 10 "
int main() {
  int a = 15;
  int b = 26;
  return a & b;
}
"

assert 11 "
int main() {
  int a = 22;
  int b = 29;
  int c = a ^ b;
  return c;
}
"

assert 61 "
int main() {
  int a = 41;
  int b = 28;
  return a | b;
}
"

assert 5 "
int main() {
  int ans = 0;
  int a = 2;
  int b = 3;
  if (a == 2 && b == 3) {
    ans = 5;
  }
  return ans;
}
"

assert 3 "
int main() {
  int ans = 0;
  int a = 2;
  int b = 3;
  if (a + b == 5 && a == 3) {
    ans = 5;
  } else {
    ans = 3;
  }
  return ans;
}
"

assert 10 "
int main() {
  int ans = 0;
  int a = 2;
  int b = 3;
  if (a == 3 || a + b == 5 || b == 2) {
    ans = 10;
  }
  return ans;
}
"

assert 2 "
int main() {
  int ans = 0;
  int a = 3;
  int b = 2;
  if (a == b || a * b == a || (a | b) == 2) {
    ans = 5;
  } else {
    ans = 2;
  }
  return ans;
}
"

assert 3 "
int main() {
  int ans = 0;
  int a = 2;
  int b = 1;
  ans = (a - b == 1 ? 3 : 2);
  return ans;
}
"

assert 5 "
int main() {
  int ans = 0;
  int a = 3;
  int b = 5;
  ans = (a + b == 7 ? ans == 0 ? 1 : 2 : a + b == 8 ? b : a);
  return ans;
}
"

assert 12 "
int main() {
  int a = 2;
  int b = 4;
  a = b = 6;
  return a + b;
}
"

assert 10 "
int main() {
  int a = 2;
  a += 5;
  return a + 3;
}
"

assert 18 "
int main() {
  int a = 2;
  int b = 3;
  a += b += 5;
  return a + b;
}
"

assert 5 "
int main() {
  int a = 10;
  a -= 7;
  return a + 2;
}
"

assert 10 "
int main() {
  int a = 10;
  int b = 3;
  int c = 7;
  a -= c -= b;
  return a + c;
}
"

assert 9 "
int main() {
  int a = 3;
  int b = 5;
  a += b -= 2;
  return a + b;
}
"

assert 15 "
int main() {
  int a = 2;
  int b = 3;
  a *= b += 2;
  return a + b;
}
"

assert 18 "
int main() {
  int a = 3;
  int b = 2;
  a *= b *= 3;
  return a;
}
"

assert 3 "
int main() {
  int a = 6;
  int b = 4;
  a /= b /= 2;
  return a;
}
"

assert 10 "
int main() {
  long a = 30;
  long b = 6;
  a /= b /= 2;
  return a;
}
"

assert 10 "
int main() {
  int a = 3;
  int b = 5;
  a %= b %= 3;
  return a + 9;
}
"

assert 32 "
int main() {
  int a = 2;
  a <<= 4;
  return a;
}
"

assert 4 "
int main() {
  int a = 128;
  a >>= 5;
  return a;
}
"

assert 20 "
int main() {
  int a = 2;
  int b = 1;
  b <<= a <<= 1;
  return b + a;
}
"

assert 5 "
int main() {
  int a = 5;
  int b = 15;
  a &= b &= 29;
  return a;
}
"

assert 23 "
int main() {
  int a = 5;
  int b = 15;
  a ^= b ^= 29;
  return a;
}
"

assert 31 "
int main() {
  int a = 5;
  int b = 15;
  a |= b |= 29;
  return a;
}
"

assert 9 "
int main() {
  int a = 4;
  int b = a++;
  return a + b;
}
"

assert 9 "
int main() {
  int a = 4;
  int b = a++ + a++;
  return b;
}
"

assert 10 "
int main() {
  int a = 5;
  int b = 4;
  if (!(a == b)) {
    b = 5;
  }
  return a + b;
}
"

assert 9 "
int main() {
  int a = 5;
  int b = 4;
  if (!(a != b)) {
    b = 5;
  }
  return a + b;
}
"

assert 30 "
int main() {
  int a = 30;
  int b = a & (~a);
  return a + b;
}
"

assert 10 "
int main() {
  int a = 2;
  int b = 5;
  {
    int b = 3;
    a += b;
  }
  return a + b;
}
"

assert 5 "
int main() {
  int a = 2, b = 3;
  return a + b;
}
"

assert 15 "
int main() {
  int *a, b = 2;
  int c = 5, d;
  a = &d;
  d = 4;
  return *a + b + c + d;
}
"

assert 2 "
int main() {
  char c = 2;
  return c;
}
"

assert 7 "
int main() {
  char a = 2;
  int b = 5;
  return a + b;

}
"

assert 4 "
int main() {
  int x = 9;
  return sizeof(x);
}
"

assert 4 "
int main() {
  int x = 9;
  return sizeof(x + 2);
}
"

assert 8 "
int main() {
  long x = 4;
  return sizeof(x);
}
"

assert 8 "
int main() {
  int x = 3;
  int *y = &x;
  return sizeof(y);
}
"

assert 4 "
int main() {
  int x = 3;
  int *y = &x;
  return sizeof(*y);
}
"

assert 32 "
int main() {
  int x[8];
  x[2] = 4;
  return sizeof(x);
}
"

assert 4 "
int main() {
  int x[8];
  x[2] = 4;
  return sizeof(x[2]);
}
"

assert 32 "
int main() {
  long x[2][2];
  return sizeof(x);
}
"

assert 2 "
int a;
int main() {
  a = 2;
  return a;
}
"

assert 10 "
int a;
int b;
int main() {
  a = 5;
  b = 5;
  return a + b;
}
"

assert 5 "
int a, b;
int main() {
  b = 1;
  int *c = &a;
  *c = 4;
  return a + b;
}
"

assert 4 "
int a[2][2];
int main() {
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      a[i][j] = i + j;
    }
  }
  return a[0][0] + a[1][1] + a[0][1] + a[1][0];
}
"

