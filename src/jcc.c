#include "asmgen/asmgen.h"
#include "parser/parser.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Invalid arguments.\n");
    fprintf(stderr, "Usage: jcc <input_path> <output_path>\n");
    exit(1);
  }

  init_type();

  Token *tkn = tokenize(argv[1]);
  Node *head = parser(tkn);
  asmgen(head, argv[2]);
}
