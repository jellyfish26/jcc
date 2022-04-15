#pragma once
#include <stdarg.h>
#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>

//
// util.c
//

char *erase_bslash_str(char *ptr, int len);
bool isident(char c);

//
// file.c
//

typedef struct {
  char *name;
  char *contents;
} File;

File *new_file(char *name, char *contents);
File *read_file(char *path);

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

//
// error.c
//

typedef enum {
  ER_COMPILE,  // Compiler Error
  ER_TOKENIZE, // Tokenize Error
  ER_INTERNAL, // Internal Error
  ER_NOTE,     // Note
} ERROR_TYPE;

void errorf(ERROR_TYPE type, char *format, ...);
void verrorf(ERROR_TYPE type, char *format, va_list ap);
void errorf_at(ERROR_TYPE type, File *file, char *loc, int underline_len, char *fmt, ...);
void verrorf_at(ERROR_TYPE type, File *file, char *loc, int underline_len, char *fmt, va_list ap);
