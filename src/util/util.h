#pragma once

//
// hashmap.c
//

#include <stdint.h>

typedef struct {
  char *key;
  int keylen;
  void *item;
} HashBucket;

typedef struct {
  HashBucket *buckets;
  int used;
  int tombstone;
  int capacity;
} HashMap;

void hashmap_insert(HashMap *map, char *key, void *item);
void hashmap_ninsert(HashMap *map, char *key, int keylen, void *item);
void *hashmap_get(HashMap *map, char *key);
void *hashmap_nget(HashMap *map, char *key, int keylen);
void hashmap_delete(HashMap *map, char *key);
void hashmap_ndelete(HashMap *map, char *key, int keylen);
