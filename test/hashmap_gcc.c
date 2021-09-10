#include "util/util.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

char *genkey(int i) {
  char *str = calloc(30, sizeof(char));
  sprintf(str, "key %d", i);
  return str;
}

int *genval(int i) {
  int *ptr = calloc(1, sizeof(int));
  *ptr = i;
  return ptr;
}

int main() {
  HashMap *map = calloc(1, sizeof(HashMap));
  assert(hashmap_get(map, "hello") == NULL);

  for (int i = 0; i < 4096; i++) {
    hashmap_insert(map, genkey(i), genval(i));
  }

  for (int i = 0; i < 2048; i++) {
    hashmap_delete(map, genkey(i));
  }

  for (int i = 1024; i < 8192; i++) {
    hashmap_insert(map ,genkey(i), genval(i));
  }

  for (int i = 10000; i < 11000; i++) {
    hashmap_insert(map, genkey(i), genval(i));
  }

  for (int i = 0; i < 1024; i++) {
    assert(hashmap_get(map, genkey(i)) == NULL);
  }

  for (int i = 1024; i < 8192; i++) {
    assert(hashmap_get(map, genkey(i)) != NULL);
    assert(*((int*)hashmap_get(map, genkey(i))) == i);
  }

  for (int i = 8192; i < 10000; i++) {
    assert(hashmap_get(map, genkey(i)) == NULL);
  }

  for (int i = 10000; i < 11000; i++) {
    assert(hashmap_get(map, genkey(i)) != NULL);
    assert(*((int*)hashmap_get(map, genkey(i))) == i);
  }

  for (int i = 11000; i < 12000; i++) {
    assert(hashmap_get(map, genkey(i)) == NULL);
  }

  assert(hashmap_get(map, "hello") == NULL);
  printf("Hashmap check passed.\n");

  return 0;
}

