#include "write.h"

void errorf(ERROR_TYPE type, char *fmt, ...) {
    va_list ap;
    va_start(ap ,fmt);
    char *err_type;
    switch (type) {
    case ER_TOKENIZE:
        err_type = "Tokenize Error";
        break;
    case ER_COMPILE:
        err_type = "Compile Error";
        break;
    case ER_OTHER:
        err_type = "Other Error";
        break;
    }
    fprintf(stderr, "\x1b[31m[%s]\x1b[39m: ", err_type);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}