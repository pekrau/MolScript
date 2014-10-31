/* key_value

   Key/value pairs of dynstrings, storable in single-linke lists.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
    14-Feb-1997  first attempt
     4-Apr-1997  use dynstring
     4-Jun-1998  mod's for hgen
*/

#include "key_value.h"

/* public ====================
#include <dynstring.h>

typedef struct s_key_value key_value;

struct s_key_value {
  dynstring *key;
  dynstring *value;
  key_value *next;
};
==================== public */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <str_utils.h>


/*------------------------------------------------------------*/
key_value *
kv_create (const char *k, const char *v)
     /*
       Create a key/value record, where the key and value are
       dynstring copies of the given strings.
     */
{
  key_value *new = malloc (sizeof (key_value));

  new->key = ds_create (k);
  new->value = ds_create (v);
  new->next = NULL;

  return new;
}


/*------------------------------------------------------------*/
void
kv_delete (key_value *kv)
     /*
       Delete this key/value record, and recursively all in the
       linked list from this.
     */
{
  /* pre */
  assert (kv);

  if (kv->next) kv_delete (kv->next);
  ds_delete (kv->key);
  ds_delete (kv->value);
  free (kv);
}


/*------------------------------------------------------------*/
key_value *
kv_append (key_value *kv, key_value *new)
     /*
       Append the given new key/value record to the end of the
       linked list from kv. The new record is returned.
     */
{
  /* pre */
  assert (kv);
  assert (new);
  assert (new->next == NULL);

  while (kv->next) kv = kv->next;
  kv->next = new;

  return new;
}


/*------------------------------------------------------------*/
key_value *
kv_search (key_value *first, const char *key)
     /*
       Return the key/value record from the given linked list having
       an equal key string as the given key. Return NULL if not found.
     */
{
  /* pre */
  assert (first);
  assert (key);

  for (; first; first = first->next) {
    if (str_eq (first->key->string, key)) return first;
  }

  return NULL;
}


/*------------------------------------------------------------*/
char *
kv_search_value_str (key_value *first, char *key)
     /*
       Return the value string of the key/value record from the given
       linked list having an equal key string as the given key.
       Return NULL if not found.
     */
{
  /* pre */
  assert (first);
  assert (key);

  for (; first; first = first->next) {
    if (str_eq (first->key->string, key)) {
      if (first->value) {
	return first->value->string;
      } else {
	return NULL;
      }
    }
  }

  return NULL;
}
