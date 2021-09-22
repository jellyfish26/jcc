#include "test.h"

// #define TWO \
//   2
// 
// #define THR\
// EE 3
// 
// #define CON1(expr) expr ## \
//   expr
// 
// #define CON2(expr) expr##_hello

int main() {
  CHECK(5, ({
    int abc\
de = 2;
    abcde + 3;
  }));

//   CHECK(16, __LI\
// NE__);

  /// CHECK(3, THREE);

  return 0;
}

