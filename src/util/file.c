#include "util/util.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

File *new_file(char *name, char *contents) {
  File *file = calloc(1, sizeof(File));
  file->name = name;
  file->contents = contents;
  return file;
}

File *read_file(char *path) {
  FILE *fp;
  if ((fp = fopen(path, "r")) == NULL) {
    fprintf(stderr, "Failed to open the file: %s\n", path);
    exit(1);
  }

  char *buf;
  size_t buflen;
  FILE *buffp = open_memstream(&buf, &buflen);

  // Read file
  while (true) {
    char tmp[1024];
    int len = fread(tmp, sizeof(char), sizeof(tmp), fp);
    if (len == 0) {
      break;
    }
    fwrite(tmp, sizeof(char), len, buffp);
  }
  fclose(fp);
  fflush(buffp);

  // To make processing easier, insert '\n' if there is not '\n' at the end.
  if (buflen == 0 || buf[buflen - 1] != '\n') {
    fputc('\n', buffp);
  }
  fputc('\0', buffp);
  fclose(buffp);

  return new_file(path, buf);
}
