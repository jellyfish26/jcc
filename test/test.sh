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

compile() {
  gcc -E -P -C $1 > $1.tmp
  ../jcc $1.tmp $1.s
  gcc -static -g -o tmp $2 common.o $1.s
  rm $1.s $1.tmp
}

compile_only_jcc() {
  ../jcc $1.c $1.s
  gcc -static -g -o tmp common.o $1.s
  rm $1.s
}

# Check hashmap
gcc -std=c11 -static -c -o common.o common_gcc.c
gcc -std=c11 -static -c -o hashmap.o hashmap_gcc.c -I ../src

gcc -static -g -o tmp ../src/util/hashmap.o hashmap.o
check "hashmap.c"

# Check function ABI
gcc -std=c11 -static -c -o function_abi_gcc.o function_abi_gcc.c
compile function_abi_jcc.c function_abi_gcc.o
rm function_abi_gcc.o
check function_abi_jcc.c

# Check macro
compile_only_jcc macro_jcc
check macro_jcc.c

# Check include
compile_only_jcc include1_jcc
check include1_jcc.c

compile_only_jcc include2_jcc
check include2_jcc.c

for src_file in `\find . -name '*.c' -not -name '*jcc.c' -not -name '*gcc.c' -not -name 'function_abi.c'`; do
  compile $src_file
  check $src_file
done
rm *.o
