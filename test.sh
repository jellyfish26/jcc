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