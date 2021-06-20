#include "code/codegen.h"
#include "parser/parser.h"
#include "token/tokenize.h"
#include <stdio.h>

int main(int argc, char **argv) {
  tokenize(argv[1]);
  program();
  codegen();
}
