#include "code/codegen.h"
#include "parser/parser.h"
#include "read/readsrc.h"
#include "token/tokenize.h"

int main(int argc, char **argv) {
  readfile(argv[1]);
  tokenize();
  program();

  codegen();
}
