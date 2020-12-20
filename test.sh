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

assert 5 "2 + 3"
assert 3 "5 - 2"
assert 4 "2 * 2"
assert 4 "8 / 2"
assert 5 "2 * 2 + 1 * 1"
assert 10 "2 * 2 / 2 + 8 / 2 + 6 - 2"