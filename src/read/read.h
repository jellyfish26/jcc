#pragma once
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// readsrc.c
//

typedef struct SourceLine SourceLine;

struct SourceLine {
  char *str;        // String
  int len;          // Length of string
  int number;       // Number of lines
  SourceLine *next; // Next line
};

extern SourceLine *source_code;
void readfile(char *file_path);
