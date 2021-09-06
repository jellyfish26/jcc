// This is an implementatin of the open addressing HashTable.

#include "util/util.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Bucket size of initialize (include update operation)
#define INIT_SIZE 128

// When the usage reaches 75%, we need to update the HashMap to a larger size.
#define MAX_USAGE 75

// When update a HashMap, keep the usage below 50%.
#define LOW_USAGE 50

// Replace delete item
#define TOMBSTONE ((void *)-1)

static uint64_t fnv1hash(char *str, int len) {
  uint64_t hash = 14695981039346656037u;
  for (int i = 0; i < len; i++) {
    hash *= 1099511628211u;
    hash ^= (unsigned char)str[i];
  }

  return hash;
}

static void hashmap_update(HashMap *map) {
  int capacity = map->capacity;
  while ((map->used - map->tombstone) * 100 / capacity >= LOW_USAGE) {
    capacity += INIT_SIZE;
  }

  HashMap new_map = {};
  new_map.buckets = calloc(capacity, sizeof(HashBucket));
  new_map.capacity = capacity;

  for (int i = 0; i < map->capacity; i++) {
    HashBucket *bucket = &(map->buckets[i]);

    if (bucket->key != NULL && bucket->key != TOMBSTONE) {
      hashmap_ninsert(&new_map, bucket->key, bucket->keylen, bucket->item);
    }
  }

  free(map->buckets);
  *map = new_map;
}

void hashmap_insert(HashMap *map, char *key, void *item) {
  hashmap_ninsert(map, key, strlen(key), item);
}

void hashmap_ninsert(HashMap *map, char *key, int keylen, void *item) {
  if (map->buckets == NULL) {
    map->buckets = calloc(INIT_SIZE, sizeof(HashBucket));
    map->capacity = INIT_SIZE;
  } else if ((map->used - map->tombstone) * 100 / map->capacity >= MAX_USAGE) {
    hashmap_update(map);
  }

  uint64_t hash = fnv1hash(key, keylen);

  for (int i = 0; i < map->capacity; i++) {
    HashBucket *bucket = &(map->buckets[(hash + i) % map->capacity]);

    if (bucket->key == NULL) {
      bucket->key = key;
      bucket->keylen = keylen;
      bucket->item = item;
      map->used++;
      return;
    }

    if (bucket->key == TOMBSTONE) {
      bucket->key = key;
      bucket->keylen = keylen;
      bucket->item = item;
      map->tombstone--;
      return;
    }

    if (strncmp(bucket->key, key, keylen) == 0 && bucket->keylen == keylen) {
      bucket->item = item;
      return;
    }
  }
}

static HashBucket *hashmap_get_bucket(HashMap *map, char *key, int keylen) {
  uint64_t hash = fnv1hash(key, keylen);

  for (int i = 0; i < map->capacity; i++) {
    HashBucket *bucket = &(map->buckets[(hash + i) % map->capacity]);

    if (bucket->key == NULL) {
      return NULL;
    }

    if (bucket->key == TOMBSTONE) {
      continue;
    }

    if (strncmp(bucket->key, key, keylen) == 0 && bucket->keylen == keylen) {
      return bucket;
    }
  }

  return NULL;
}

void *hashmap_get(HashMap *map, char *key) {
  return hashmap_nget(map, key, strlen(key));
}

void *hashmap_nget(HashMap *map, char *key, int keylen) {
  HashBucket *bucket = hashmap_get_bucket(map, key, keylen);

  if (bucket != NULL) {
    return bucket->item;
  }
  return NULL;
}

void hashmap_delete(HashMap *map, char *key) {
  hashmap_ndelete(map, key, strlen(key));
}

void hashmap_ndelete(HashMap *map, char *key, int keylen) {
  HashBucket *bucket = hashmap_get_bucket(map, key, keylen);

  if (bucket != NULL) {
    bucket->key = TOMBSTONE;
    map->tombstone++;
  }
}
