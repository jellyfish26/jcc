#include "jcc.h"

int main(int argc, char **argv) {
    readfile(argv[1]);
    tokenize();
    program();

    codegen();
}