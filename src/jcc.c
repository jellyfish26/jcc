#include "asmgen/asmgen.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Invalid arguments.\n");
    fprintf(stderr, "Usage: jcc <expr> <output_file>\n");
    exit(1);
  }

  Token *tkn = tokenize(argv[1]);
  Node *head = parser(tkn);
  asmgen(head, argv[2]);
}
