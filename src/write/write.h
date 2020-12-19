#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//
// error.c
//

typedef enum {
    ER_COMPILE,  // Compiler Error
    ER_TOKENIZE, // Tokenize Error
    ER_OTHER,    // Other Error
} ERROR_TYPE;

void errorf(ERROR_TYPE type, char *format, ...);

//
// codegen.c
//

void codegen();