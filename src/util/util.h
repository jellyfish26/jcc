#pragma once
#define _POSIX_C_SOURCE 200809L

//
// file.c
//

typedef struct {
  char *name;
  char *contents;
} File;

File *new_file(char *name, char *contents);
File *read_file(char *path);
