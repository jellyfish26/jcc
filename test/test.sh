check() {
  gcc -std=c11 -static -c -o common.o common.c
  for src_file in `\find . -name '*.c' -not -name '*common.c'`; do
    gcc -E -P -C $src_file > $src_file.tmp
    ../jcc $src_file.tmp $src_file.s
    gcc -static -o tmp common.o $src_file.s
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
}

check
