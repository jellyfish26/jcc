#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

//
// readsrc.c
//

typedef struct SourceLine SourceLine;

struct SourceLine {
    char *str;         // String 
    int len;           // Length of string
    int number;        // Number of lines
    SourceLine *next;  // Next line
};

extern SourceLine *source_code;
void readfile(char *file_path);