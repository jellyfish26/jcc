#include "code/codegen.h"
#include "parser/parser.h"
#include "token/tokenize.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Invalid arguments.\n");
    fprintf(stderr, "Usage: jcc <input_file> <output_file>\n");
    exit(1);
  }
  init_type();

  Token *tkn = tokenize(argv[1]);
  Node *head = program(tkn);
  codegen(head, argv[2]);
}
