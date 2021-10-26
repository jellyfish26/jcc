#!/bin/sh

check() {
  ./tmp
  if [ $? -eq 0 ]; then
    echo "test $1 passed."
    rm tmp
  else
    echo "test $1 failed."
    rm tmp
    exit 1
  fi
}

gcc -std=c11 -static -c -o hashmap.o hashmap_gcc.c -I ../src
gcc -static -g -o tmp ../src/util/hashmap.o hashmap.o
rm hashmap.o
check "hashmap.c"

assert() {
  expected="$1"
  input="$2"

  echo "$input" > tmp.c
  ../jcc tmp.c tmp.s
  gcc -static -o tmp tmp.s
  ./tmp
  actual="$?"

  # rm tmp tmp.s tmp.c
  if [ "$actual" = "$expected" ]; then
    echo -e "\e[32m[ OK ]\e[m $input => $actual"
  else
    echo -e "\e[31m[ NG ]\e[m $input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 3  "int main() { return 1+2; }"
assert 1  "int main() { return 2-1; }"
assert 8  "int main() { return 2*4; }"
assert 3  "int main() { return 6/2; }"
assert 10 "int main() { return 2*4+2; }"
assert 1  "int main() { return 5%2; }"
assert 49 "int main() { return (2+5)*7; }"
assert 1  "int main() { return (2+5)%(3-1); }"
assert 6  "int main() { return 3<<1; }"
assert 32 "int main() { return 8<<2; }"
assert 1  "int main() { return 3>>1; }"
assert 4  "int main() { return 8>>1; }"

assert $((1 > 0))  "int main() { return 1 > 0; }"
assert $((1 < 0))  "int main() { return 1 < 0; }"
assert $((0 > 1))  "int main() { return 0 > 1; }"
assert $((0 < 1))  "int main() { return 0 < 1; }"
assert $((1 <= 1)) "int main() { return 1 <= 1; }"
assert $((1 <= 0)) "int main() { return 1 <= 0; }"
assert $((0 <= 1)) "int main() { return 0 <= 1; }"
assert $((1 >= 1)) "int main() { return 1 >= 1; }"
assert $((1 >= 0)) "int main() { return 1 >= 0; }"
assert $((0 >= 1)) "int main() { return 0 >= 1; }"
assert $((0 == 0)) "int main() { return 0 == 0; }"
assert $((0 == 1)) "int main() { return 0 == 1; }"
assert $((0 != 0)) "int main() { return 0 != 0; }"
assert $((0 != 1)) "int main() { return 0 != 1; }"

assert $((42 & 32)) "int main() { return 42 & 32; }"
assert $((13 & 2))  "int main() { return 13 & 2; }"
assert $((24 & 42)) "int main() { return 24 & 42; }"

assert $((42 ^ 32)) "int main() { return 42 ^ 32; }"
assert $((13 ^ 2))  "int main() { return 13 ^ 2; }"
assert $((24 ^ 42)) "int main() { return 24 ^ 42; }"

assert $((42 | 32)) "int main() { return 42 | 32; }"
assert $((13 | 2))  "int main() { return 13 | 2; }"
assert $((24 | 42)) "int main() { return 24 | 42; }"

assert $((0 && 0)) "int main() { return 0 && 0; }"
assert $((0 && 1)) "int main() { return 0 && 1; }"
assert $((1 && 1)) "int main() { return 1 && 1; }"

assert $((0 || 0)) "int main() { return 0 || 0; }"
assert $((0 || 1)) "int main() { return 0 || 1; }"
assert $((1 || 1)) "int main() { return 1 || 1; }"

assert 2 "int main() { return 0 ? 1 : 2; }"
assert 3 "int main() { return 0 || 1 ? 3 : 1; }"
assert 1 "int main() { return 0 && 1 ? 3 : 1; }"
assert 5 "int main() { return 1 ? 1 ? 5 : 3 : 2; }"
assert 2 "int main() { return 0 ? 3 : 0 ? 6 : 2; }"

assert 5 "int main() { 1 + 2; return 3 + 2; }"
assert 5 "int main() { int a; return 3 + 2; }"
assert 5 "int main() { int a; int b; return 3 + 2; }"
assert 3 "int main() { return 1 + 2; return 3 + 2; }"

assert 5 "int main() { int a; a = 2; return 3 + a; }"
assert 6 "int main() { int a; int b; a = b = 3; return a + b; }"
assert 5 "int main() { int a; a = 3; a = a + 2; return a; }"
assert 5 "int main() { int a; a = 3; a += 2; return a; }"
assert 6 "int main() { int a; int b; a = b = 3; a += 2; b -= 2; return a + b; }"
assert 18 "int main() { int a; int b; a = 24; b = 2; b += a &= 42; return a + b; }"

assert 5 "int main() { int a; int b; a = 2; b = a++; return a + b; }"
assert 3 "int main() { int a; int b; a = 2; b = a--; return a + b; }"
assert 6 "int main() { int a; int b; a = 2; b = ++a; return a + b; }"
assert 2 "int main() { int a; int b; a = 2; b = --a; return a + b; }"
assert 240 "int main() { char a; a = 240; return a; }"

assert 5 "int main() { int *a; int b; b = 5; a = &b; return *a; }"
assert 3 "int main() { int *a; int b; b = 5; a = &b; *a = 3; return b; }"

assert 8 "int main() { int a; a = ({int a; int b; a = 3; b = 5; a + b;}); return a; }"
assert 2 "int main() { int a; a = 2; return +(+(a)); }"
assert 2 "int main() { int a; a = 2; return -(-(a)); }"

assert 1 "int main() { return ~254; }"
assert 1 "int main() { return !0; }"
assert 0 "int main() { return !1; }"
assert 3 "
int add2(int a, int b) {
  return a + b;
}

int main() {
  return add2(1, 2);
}
"
