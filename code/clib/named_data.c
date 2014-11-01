/*
   Named data record definition and routines.

   Simple storage of a memory block with a name in a node,
   which may be stored in a linear list of nodes.

   clib v1.1

   Copyright (C) 1998 Per Kraulis
    19-Jan-1998  first attempt
     3-Apr-1998  added data clone procedure
*/

#include "named_data.h"

/* public ====================
#include <stdlib.h>

#include <boolean.h>

typedef struct _named_data named_data;

struct _named_data {
  char *name;
  void *data;
  boolean name_copy, data_copy;
  named_data *next;
};
==================== public */

#include <string.h>
#include <assert.h>

#include <str_utils.h>


/*------------------------------------------------------------*/
named_data *
nd_create (char *name, boolean copy)
{
  named_data *new = malloc (sizeof (named_data));

  /* pre */
  assert (name);

  new->name = name;
  new->name_copy = copy;
  new->data = NULL;
  new->data_copy = FALSE;
  new->next = NULL;

  return new;
}


/*------------------------------------------------------------*/
named_data *
nd_set_data (named_data *nd, void *data, boolean copy)
{
  /* pre */
  assert (nd);

  if (nd->data && nd->data_copy) free (nd->data);
  nd->data = data;
  nd->data_copy = copy;

  return nd;
}


/*------------------------------------------------------------*/
named_data *
nd_set_data_clone (named_data *nd, void *data, size_t size)
{
  /* pre */
  assert (nd);

  if (nd->data && nd->data_copy) free (nd->data);
  nd->data = memcpy (malloc (size), data, size);
  nd->data_copy = TRUE;

  return nd;
}


/*------------------------------------------------------------*/
void
nd_delete (named_data *nd)
{
  /* pre */
  assert (nd);

  if (nd->next) nd_delete (nd->next);

  if (nd->name_copy) free (nd->name);
  if (nd->data && nd->data_copy) free (nd->data);
  free (nd);
}


/*------------------------------------------------------------*/
named_data *
nd_append (named_data *nd, named_data *new)
{
  /* pre */
  assert (nd);
  assert (new);

  while (nd->next) nd = nd->next;
  nd->next = new;

  return new;
}


/*------------------------------------------------------------*/
named_data *
nd_search (named_data *nd, const char *name)
{
  /* pre */
  assert (name);

  for ( ; nd; nd = nd->next) {
    assert (nd->name);
    if (str_eq (nd->name, name)) return nd;
  }

  return NULL;
}


/*------------------------------------------------------------*/
void *
nd_search_data (named_data *nd, const char *name)
{
  /* pre */
  assert (name);

  for ( ; nd; nd = nd->next) {
    assert (nd->name);
    if (str_eq (nd->name, name)) return nd->data;
  }

  return NULL;
}
