/* double_hash.h

   Hash table for objects using string keys, using the double-hashing
   scheme. No removal of single entries can be done in this implementation.

   The key and object pointers are assumed to point to memory shared with
   other parts of the calling program. No duplication or cleanup of the
   memory for these entities is performed by the procedures in this package,
   except by the explicit procedure 'dhash_delete_contents'.

   clib v1.0

   Copyright (C) 1997 Per Kraulis
     27-Aug-1997  first attempts
*/

#ifndef DOUBLE_HASH_H
#define DOUBLE_HASH_H 1

#include <boolean.h>

typedef struct {
  char **keys;
  void **objects;
  int count;
  int size;
  double auto_resize;
} dhash_table;

dhash_table *dhash_create (const int max_entries);
dhash_table *dhash_create_size (const int size);
void dhash_resize (dhash_table *table, const int new);
void dhash_delete (dhash_table *table);
void dhash_delete_contents (dhash_table *table);

int dhash_insert (dhash_table *table, char *key, void *object);
void dhash_insert_replace (dhash_table *table, char *key, void *object);

int dhash_slot (const dhash_table *table, const char *key);
void *dhash_object (const dhash_table *table, const char *key);

#endif
