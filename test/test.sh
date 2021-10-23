#!/bin/sh


compile() {
  echo $1 > tmp.c
  ../jcc tmp.c $3.s
  gcc -static -g -o tmp $3.s
  rm tmp.c $3.s
  ./tmp
  if [ $? -eq $2 ]; then
    echo "test $3 passed."
    rm tmp
  else
    echo "test $3 failed."
    rm tmp
    exit 1
  fi
}

compile 1+2 3 plus
compile 2-1 1 minus
compile 2*4 8 mul1
compile 6/2 3 div1
compile 2*4+2 10 mul2
compile 5%2 1 mod1
