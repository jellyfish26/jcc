assert() {
  expected="$1"
  input="$2"

  echo "$input" > tmp.c

  ./jcc tmp.c > tmp.s
  cc -static -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo -e "\e[32m[ OK ]\e[m $input => $actual"
  else
    echo -e "\e[31m[ NG ]\e[m $input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 5  "return 2 + 3;"
assert 3  "return 5 - 2;"
assert 4  "return 2 * 2;"
assert 4  "return 8 / 2;"
assert 5  "return 2 * 2 + 1 * 1;"
assert 10 "return 2 * 2 / 2 + 8 / 2 + 6 - 2;"
assert 5  "return (2 + 3) * (3 - 2);"
assert 14 "return ((2 * 3 - 2) / 2) * (1 + 3 * 2);"

assert 1 "return -2 + 3;"
assert 6 "return +2 + +2 - -2;"

assert $((1 == 1)) "return 1 == 1;"
assert $((1 <= 1)) "return 1 <= 1;"
assert $((1 >= 1)) "return 1 >= 1;"
assert $((1 > 0))  "return 1 > 0;"
assert $((1 < 0))  "return 1 < 0;"
assert $((0 > 1))  "return 0 > 1;"
assert $((0 < 1))  "return 0 < 1;"
assert $((1 >= 0)) "return 1 >= 0;"
assert $((1 <= 0)) "return 1 <= 0;"
assert $((0 >= 1)) "return 0 >= 1;"
assert $((0 <= 1)) "return 0 <= 1;"
assert $((0 != 1)) "return 0 != 1;"

assert 2 "return (1 == 1) + (0 != 1);"
assert 2 "return (3 <= 3) + (3 < 3) + (4 > 4) + (4 >= 4);"
assert 1 "return (2 * 3 > 2 * 2) + (2 * 3 <= 2 * 2);"

assert 2 "hoge; hoge = 2; return hoge;"
assert 3 "test = 2; test = 3; return test;"
assert 6 "foo = 2; bar = 3; return foo * bar;"

assert 2 "hoge = 3; if (hoge == 3) hoge = 2; return hoge;"
assert 3 "if (2 == 3) return 2; return 3;"
assert 4 "hoge = 2; fuga = 4; if (hoge == fuga) return hoge; return fuga;"
assert 5 "foo = 2; bar = 3; if (foo < bar) foo = foo + bar; return foo;"
assert 2 "foo = 2; bar = 3; if (foo > bar) foo = foo + bar; return foo;"

assert 2 "foo = 0; if (foo == 0) foo = 2; else foo = 3; return foo;"
assert 3 "foo = 0; if (foo != 0) foo = 2; else foo = 3; return foo;"

assert 4 "hoge = 2; fuga = 3; if (hoge == 2) { fuga = 2; hoge = hoge * fuga; } else { fuga = 5; hoge = fuga - hoge; } return hoge;"
assert 3 "hoge = 2; fuga = 3; if (hoge == 1) { fuga = 2; hoge = hoge * fuga; } else { fuga = 5; hoge = fuga - hoge; } return hoge;"

assert 2 "hoge = 3; if (hoge == 2) hoge = 1; else if (hoge == 3) hoge = 2; else hoge = 3; return hoge;"