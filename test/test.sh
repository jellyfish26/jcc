#!/bin/sh
gcc -std=c11 -static -c -o common.o common.c
gcc -std=c11 -static -c -o hashmap.o hashmap.c -I ../src

gcc -static -g -o tmp ../src/util/hashmap.o hashmap.o
./tmp

if [ $? -eq 0 ]; then
  rm tmp hashmap.o
else
  rm tmp hashmap.o
  echo "Hashmap check failed."
  exit 1
fi

for src_file in `\find . -name '*.c' -not -name '*common.c' -not -name 'hashmap.c'`; do
  gcc -E -P -C $src_file > $src_file.tmp
  ../jcc $src_file.tmp $src_file.s
  gcc -static -g -o tmp common.o $src_file.s
  rm $src_file.s $src_file.tmp
  ./tmp
  if [ $? -eq 0 ]; then
    echo "test $src_file passed."
    rm tmp
  else
    echo "test $src_file failed."
    rm tmp
    exit 1
  fi
done
