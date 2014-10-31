/* double_hash.c

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

#include "double_hash.h"

#include <assert.h>
#include <stdlib.h>

#include <str_utils.h>


#define H2(s) (8 - ((s) % 8))


/*------------------------------------------------------------*/
dhash_table *
dhash_create (const int max_entries)
{
  assert (max_entries > 0);

  return dhash_create_size ((int) (max_entries / 0.666666));
}


/*------------------------------------------------------------*/
dhash_table *
dhash_create_size (const int size)
{
  dhash_table *new;

  assert (size > 0);

  new = malloc (sizeof (dhash_table));
  new->count = 0;
  new->size = size;
  new->keys = calloc (new->size, sizeof (char *));
  new->objects = calloc (new->size, sizeof (void *));
  new->auto_resize = 0.666666;

  return new;
}


/*------------------------------------------------------------*/
void
dhash_resize (dhash_table *table, const int new)
{
  char **keys;
  void **objects;
  int slot, size;

  assert (table);
  assert (new > 0);
  assert (new > table->size);

  keys = table->keys;
  objects = table->objects;
  size = table->size;

  table->count = 0;
  table->size = new;
  table->keys = calloc (table->size, sizeof (char *));
  table->objects = calloc (table->size, sizeof (void *));

  for (slot = 0; slot < size; slot++) {
    dhash_insert (table, keys[slot], objects[slot]);
  }

  free (keys);
  free (objects);

  assert (table->size == new);
}


/*------------------------------------------------------------*/
void
dhash_delete (dhash_table *table)
{
  assert (table);

  free (table->keys);
  free (table->objects);
  free (table);
}

/*------------------------------------------------------------*/
void
dhash_delete_contents (dhash_table *table)
{
  int slot;

  assert (table);

  for (slot = 0; slot < table->size; slot++) {
    if (table->keys[slot]) {
      free (table->keys[slot]);
      free (table->objects[slot]);
    }
  }

  dhash_delete (table);
}


/*------------------------------------------------------------*/
int
dhash_insert (dhash_table *table, char *key, void *object)
{
  int slot, u;

  assert (table);
  assert (key);
  assert (*key);
  assert (object);
  assert (table->size - table->count > 0);

  if ((double) table->count / (double) table->size >= table->auto_resize) {
    dhash_resize (table, 2 * table->size);
  }

  slot = str_hashcode (key, table->size);
  u = H2 (slot);
  while (table->keys[slot]) {
    if (str_eq (key, table->keys[slot])) return FALSE;
    slot = (slot + u) % table->size;
  }

  table->keys[slot] = key;
  table->objects[slot] = object;
  table->count++;

  return TRUE;
}


/*------------------------------------------------------------*/
void
dhash_insert_replace (dhash_table *table, char *key, void *object)
{
  int slot, u;

  assert (table);
  assert (key);
  assert (*key);
  assert (object);
  assert (table->size - table->count > 0);

  if ((double) table->count / (double) table->size >= table->auto_resize) {
    dhash_resize (table, 2 * table->size);
  }

  slot = str_hashcode (key, table->size);
  u = H2 (slot);
  while (table->keys[slot]) {
    if (str_eq (key, table->keys[slot])) break;
    slot = (slot + u) % table->size;
  }

  table->keys[slot] = key;
  table->objects[slot] = object;
  table->count++;
}


/*------------------------------------------------------------*/
int
dhash_slot (const dhash_table *table, const char *key)
{
  int slot, u;

  assert (table);
  assert (key);
  assert (*key);

  for (slot = str_hashcode (key, table->size), u = H2 (slot);
       ;
       slot = (slot + u) % table->size) {
    if (table->keys[slot] == NULL) {
      return -1;
    } else if (str_eq (key, table->keys[slot])) {
      return slot;
    }
  }
}


/*------------------------------------------------------------*/
void *
dhash_object (const dhash_table *table, const char *key)
{
  int slot;

  assert (table);
  assert (key);
  assert (*key);

  slot = dhash_slot (table, key);
  if (slot >= 0) {
    return table->objects[slot];
  } else {
    return NULL;
  }
}
