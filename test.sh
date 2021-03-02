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

assert 5  "main() { return 2 + 3; }"
assert 3  "main() { return 5 - 2; }"
assert 4  "main() { return 2 * 2; }"
assert 4  "main() { return 8 / 2; }"
assert 5  "main() { return 2 * 2 + 1 * 1; }"
assert 10 "main() { return 2 * 2 / 2 + 8 / 2 + 6 - 2; }"
assert 5  "main() { return (2 + 3) * (3 - 2); }"
assert 14 "main() { return ((2 * 3 - 2) / 2) * (1 + 3 * 2); }"

assert 1 "main() { return -2 + 3; }"
assert 6 "main() { return +2 + +2 - -2; }"

assert $((1 == 1)) "main() { return 1 == 1; }"
assert $((1 <= 1)) "main() { return 1 <= 1; }"
assert $((1 >= 1)) "main() { return 1 >= 1; }"
assert $((1 > 0))  "main() { return 1 > 0; }"
assert $((1 < 0))  "main() { return 1 < 0; }"
assert $((0 > 1))  "main() { return 0 > 1; }"
assert $((0 < 1))  "main() { return 0 < 1; }"
assert $((1 >= 0)) "main() { return 1 >= 0; }"
assert $((1 <= 0)) "main() { return 1 <= 0; }"
assert $((0 >= 1)) "main() { return 0 >= 1; }"
assert $((0 <= 1)) "main() { return 0 <= 1; }"
assert $((0 != 1)) "main() { return 0 != 1; }"

assert 2 "main() { return (1 == 1) + (0 != 1); }"
assert 2 "main() { return (3 <= 3) + (3 < 3) + (4 > 4) + (4 >= 4); }"
assert 1 "main() { return (2 * 3 > 2 * 2) + (2 * 3 <= 2 * 2); }"

assert 2 "main() { hoge; hoge = 2; return hoge; }"
assert 3 "main() { test = 2; test = 3; return test; }"
assert 6 "main() { foo = 2; bar = 3; return foo * bar; }"

assert 2 "main() { hoge = 3; if (hoge == 3) hoge = 2; return hoge; }"
assert 3 "main() { if (2 == 3) return 2; return 3; }"
assert 4 "main() { hoge = 2; fuga = 4; if (hoge == fuga) return hoge; return fuga; }"
assert 5 "main() { foo = 2; bar = 3; if (foo < bar) foo = foo + bar; return foo; }"
assert 2 "main() { foo = 2; bar = 3; if (foo > bar) foo = foo + bar; return foo; }"

assert 2 "main() { foo = 0; if (foo == 0) foo = 2; else foo = 3; return foo; }"
assert 3 "main() { foo = 0; if (foo != 0) foo = 2; else foo = 3; return foo; }"

assert 4 "main() { hoge = 2; fuga = 3; if (hoge == 2) { fuga = 2; hoge = hoge * fuga; } else { fuga = 5; hoge = fuga - hoge; } return hoge; }"
assert 3 "main() { hoge = 2; fuga = 3; if (hoge == 1) { fuga = 2; hoge = hoge * fuga; } else { fuga = 5; hoge = fuga - hoge; } return hoge; }"

assert 2 "main() { hoge = 3; if (hoge == 2) hoge = 1; else if (hoge == 3) hoge = 2; else hoge = 3; return hoge; }"

assert 20 "main() { i; hoge = 0; for (i = 0; i < 10; i = i + 1) { hoge = hoge + 2; } return hoge; }"
assert 20 "main() { i; hoge = 0; for (i = 0; i < 10; i = i + 1) hoge = hoge + 2; return hoge; }"
assert 2  "main() { i = 10; hoge = 22; for (; i > 0; i = i - 1) { hoge = hoge - 2; } return hoge; }"
assert 10 "main() { hoge = 0; for (i = 0; i < 10;) { hoge = hoge + 1; i = i + 1; } return hoge; }"
assert 55 "main() { hoge = 0; for (i = 1; i <= 10; i = i + 1) { hoge = hoge + i; } return hoge; }"
assert 25 "main() { hoge = 0; for (i = 0; i < 5; i = i + 1) { for (j = 0; j < 5; j = j + 1) { hoge = hoge + 1; }} return hoge; }"

assert 6  "main() { i = 0; while (i < 5) i = i + 2; return i; }"
assert 10 "main() { i = 4; hoge = 0; while (i >= 0) { i = i - 1; hoge = hoge + 2; } return hoge; }"

assert 3 "main() { hoge = 0; while (1) { hoge = hoge + 1; if (hoge == 3) break; } return hoge; }"
assert 8 "main() { hoge = 0; while (1) { hoge = hoge + 1; if (hoge <= 5) continue; hoge = hoge + 2; break; } return hoge; }"
assert 6 "main() { hoge = 0; for (i = 0; i < 5; i = i + 1) { hoge = hoge + 2; if (i >= 2) break; } return hoge; }"

stdio_test "foo" "main() { foo(); return 0; }"
stdio_test "1, 2, 3, -1, 2, 0" "main() { i = 1; j = 2; hoge(i, j, i + j, i - j, i * j, i / j); return 0; }"

stdio_test "foo" "fuga() { foo(); } main() { fuga(); return 0; }"

stdio_test "3, -1, 2, 4, -3, -3" "
test(a, b) {
  hoge(a, b, a + b, a - b, a * b, a / b); 
}

main() {
  i = 1; j = 2;
  test(i + j, i - j);
}
"

stdio_test "6, 5, 4, 3, 2, 1" "
test(a, b, c, d, e, f) {
  hoge(f, e, d, c, b, a); 
}

main() {
  a = 1; b = 2; c = 3; d = 4; e = 5; f = 6;
  test(a, b, c, d, e, f);
}
"

stdio_test "6, 5, 4, 3, 2, 1" "
test(a, b, c, d, e, f) {
  g = 3;
  hoge(f, e, d, c, b, a); 
}

main() {
  a = 1; b = 2; c = 3; d = 4; e = 5; f = 6;
  test(a, b, c, d, e, f);
}
"

stdio_test "1, 2, 3, 4, 5, 6, 7, 8" "
main() {
  a = 1; b = 2; c = 3; d = 4; e = 5; f = 6; g = 7; h = 8;
  eight_print(a, b, c, d, e, f, g, h);
}
"

