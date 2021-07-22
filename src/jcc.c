#include "code/codegen.h"
#include "parser/parser.h"
#include "token/tokenize.h"

int main(int argc, char **argv) {
  Token *tkn = tokenize(argv[1]);
  Function *head = program(tkn);
  codegen(head);
}
