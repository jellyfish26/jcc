#include "code/codegen.h"
#include "parser/parser.h"
#include "token/tokenize.h"

int main(int argc, char **argv) {
  tokenize(argv[1]);
  program();
  codegen();
}
