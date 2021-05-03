#include "parser/parser.h"
#include "read/read.h"
#include "token/token.h"
#include "write/write.h"

int main(int argc, char **argv) {
  readfile(argv[1]);
  tokenize();
  program();

  codegen();
}
