#include "code/codegen.h"
#include "parser/parser.h"
#include "token/tokenize.h"

#include <stdio.h>
#include <stdlib.h>

static void add_default_include_paths() {
  add_include_path("/usr/include/x86_64-linux-gnu");
  add_include_path("/usr/include");
  add_include_path("/usr/local/include");
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Invalid arguments.\n");
    fprintf(stderr, "Usage: jcc <input_file> <output_file>\n");
    exit(1);
  }

  add_default_include_paths();
  init_type();

  Token *tkn = tokenize(argv[1]);
  Node *head = program(tkn);
  codegen(head, argv[2]);
}
