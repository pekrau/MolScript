/* dynstring.c

   Dynamic string. The string value is a separate copy.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
     3-Apr-1997  first attempts
    20-Sep-1997  added ds_replace
    25-Nov-1997  added ds_pad_blanks
     4-Feb-1998  changed capacity alloc/realloc scheme
    26-May-1998  mod's for hgen
*/

#include "dynstring.h"

/* public ====================
#include <boolean.h>

typedef struct {
  char *string;
  int capacity;
  int length;
} dynstring;
==================== public */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/*------------------------------------------------------------*/
static void
ds_alloc (dynstring *ds, const int capacity)
{
  assert (ds);
  assert (capacity >= 0);

  ds->capacity = (capacity < 15) ? 15 : capacity;
  ds->string = malloc ((ds->capacity + 1) * sizeof (char));
}


/*------------------------------------------------------------*/
static void
ds_realloc (dynstring *ds, const int capacity)
{
  assert (ds);
  assert (capacity > 0);

  if (ds->capacity >= capacity) return;

  ds->capacity *= 2;
  if (ds->capacity < capacity) ds->capacity = capacity;
  ds->string = realloc (ds->string, (ds->capacity + 1) * sizeof (char));
}


/*------------------------------------------------------------*/
boolean
ds_valid_state (const dynstring *ds)
{
  assert (ds);

  if (ds->string == NULL) return FALSE;
  if (ds->capacity < ds->length) return FALSE;

  return (strlen (ds->string) == ds->length);
}


/*------------------------------------------------------------*/
dynstring *
ds_create (const char *str)
     /*
       Create a dynstring containing the given string, if any.
     */
{
  dynstring *new = malloc (sizeof (dynstring));

  if (str) {
    new->length = strlen (str);
    ds_alloc (new, new->length);
    strcpy (new->string, str);
  } else {
    ds_alloc (new, 15);
    new->string[0] = '\0';
    new->length = 0;
  }

  assert (ds_valid_state (new));

  return new;
}


/*------------------------------------------------------------*/
dynstring *
ds_subcreate (const char *str, int first, int last)
     /*
       Create a dynstring containing the given substring.
     */
{
  dynstring *new = malloc (sizeof (dynstring));

  /* pre */
  assert (str);
  assert (first >= 0);
  assert (last < strlen (str));
  assert (first <= last);

  new->length = last - first + 1;
  ds_alloc (new, new->length);
  strncpy (new->string, str + first, new->length);
  new->string[new->length] = '\0';

  assert (ds_valid_state (new));

  return new;
}


/*------------------------------------------------------------*/
dynstring *
ds_clone (dynstring *ds)
     /*
       Create a separate copy of the given dynstring.
     */
{
  dynstring *new = malloc (sizeof (dynstring));

  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));

  new->length = ds->length;
  ds_alloc (new, new->length);
  strcpy (new->string, ds->string);

  assert (ds_valid_state (new));

  return new;
}


/*------------------------------------------------------------*/
dynstring *
ds_subclone (dynstring *ds, int first, int last)
     /*
       Create a separate copy of a substring of the given dynstring.
     */
{
  dynstring *new = malloc (sizeof (dynstring));

  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));
  assert (first >= 0);
  assert (last < ds->length);
  assert (first <= last);

  new->length = last - first + 1;
  ds_alloc (new, new->length);
  strncpy (new->string, ds->string + first, new->length);
  new->string[new->length] = '\0';

  assert (ds_valid_state (new));

  return new;
}


/*------------------------------------------------------------*/
dynstring *
ds_allocate (const int capacity)
     /*
       Create an empty dynstring with the given capacity.
     */
{
  dynstring *new;

  /* pre */
  assert (capacity > 0);

  new = malloc (sizeof (dynstring));
  ds_alloc (new, capacity);
  new->string[0] = '\0';
  new->length = 0;

  assert (ds_valid_state (new));

  return new;
}


/*------------------------------------------------------------*/
void
ds_reallocate (dynstring *ds, const int capacity)
     /*
       Increase the capacity of the dynstring to at least the given value.
     */
{
  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));
  assert (capacity > 0);

  ds_realloc (ds, capacity);

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_delete (dynstring *ds)
{
  /* pre */
  assert (ds);

  free (ds->string);
  free (ds);
}


/*------------------------------------------------------------*/
void
ds_set (dynstring *ds, const char *str)
     /*
       Copy the given string into the dynstring.
     */
{
  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));
  assert (str);

  ds->length = strlen (str);
  ds_realloc (ds, ds->length);
  strcpy (ds->string, str);

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_subset (dynstring *ds, const char *str, int first, int last)
     /*
       Copy the given substring into the dynstring.
     */
{
  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));
  assert (str);
  assert (first >= 0);
  assert (last < strlen (str));
  assert (first <= last);

  ds->length = last - first + 1;
  ds_realloc (ds, ds->length);
  strncpy (ds->string, str + first, ds->length);
  ds->string[ds->length] = '\0';

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_cat (dynstring *ds, const char *str)
     /*
       Concatenate the given string to the dynstring.
     */
{
  register int length;

  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));
  assert (str);
  assert (ds->string != str);

  length = strlen (str);
  ds_realloc (ds, ds->length + length);
  strcpy (ds->string + ds->length, str);
  ds->length += length;

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_subcat (dynstring *ds, const char *str, int first, int last)
     /*
       Concatenate the given substring to the dynstring.
     */
{
  register int old;

  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));
  assert (str);
  assert (first >= 0);
  assert (last < strlen (str));
  assert (first <= last);

  old = ds->length;
  ds->length += last - first + 1;
  ds_realloc (ds, ds->length);
  strncpy (ds->string + old, str + first, last - first + 1);
  ds->string[ds->length] = '\0';

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_add (dynstring *ds, const char c)
     /*
       Add the character to the dynstring.
     */
{
  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));
  assert (c);

  ds_realloc (ds, ds->length + 1);
  ds->string[ds->length] = c;
  ds->length++;
  ds->string[ds->length] = '\0';

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_cat_int (dynstring *ds, const int i)
     /*
       Add the decimal string representation of the integer to the dynstring.
     */
{
  char str[40];

  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));

  sprintf (str, "%i", i);
  ds_cat (ds, str);
}


/*------------------------------------------------------------*/
void
ds_append (dynstring *ds1, const dynstring *ds2)
     /*
       Copy the contents of second dynstring to the end of the first dynstring.
     */
{
  /* pre */
  assert (ds1);
  assert (ds_valid_state (ds1));
  assert (ds2);
  assert (ds_valid_state (ds2));

  ds_realloc (ds1, ds1->length + ds2->length);
  strcpy (ds1->string + ds1->length, ds2->string);
  ds1->length += ds2->length;

  assert (ds_valid_state (ds1));
}


/*------------------------------------------------------------*/
void
ds_replace (dynstring *ds, const int pos, const int len, const char *str)
     /*
       Replace the contents of the given substring in the dynstring with
       the given string.
     */
{
  int len2, length, swap;

  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));
  assert (pos >= 0);
  assert (len >= 0);
  assert (pos + len <= ds->length);

  len2 = (str == NULL) ? 0 : strlen (str);
  length = ds->length + len2 - len;
  ds_realloc (ds, length);
  swap = ds->length - (pos + len) + 1;
  if (swap > 0) memmove (ds->string + pos + len2, ds->string + pos + len, swap);
  if (len2 > 0) memcpy (ds->string + pos, str, len2);
  ds->string[length] = '\0';
  ds->length = length;

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_fill_blanks (dynstring *ds, const int length)
     /*
       Fill the dynstring with the given number of blanks.
     */
{
  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));
  assert (length >= 0);

  ds_realloc (ds, length);
  memset (ds->string, ' ', length);
  ds->string[length] = '\0';
  ds->length = length;

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_remove_blanks (dynstring *ds)
     /*
       Remove all blanks from the dynstring.
     */
{
  register char *pos, *last;

  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));

  last = ds->string;
  pos = last;
  while (*pos) {
    if (*pos != ' ') *last++ = *pos;
    pos++;
  }
  *last = '\0';
  ds->length = last - ds->string;

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_pad_blanks (dynstring *ds, int length)
     /*
       Pad the dynstring to the right with blanks up to the given total
       length of the string.
     */
{
  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));
  assert (length >= 0);

  if (ds->length >= length) return;
  ds_realloc (ds, length);
  memset (ds->string + ds->length, ' ', length - ds->length);
  ds->string[length] = '\0';
  ds->length = length;

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_truncate (dynstring *ds, int length)
     /*
       Truncate the dynstring at the given length.
       No effect if the dynstring is already shorter.
     */
{
  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));
  assert (length >= 0);

  if (ds->length <= length) return;

  ds->string[length] = '\0';
  ds->length = length;

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_left_adjust (dynstring *ds)
     /*
       Left-adjust the dynstring; remove the first blanks.
     */
{
  register char *nonblank;

  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));

  nonblank = ds->string;
  while (*nonblank && (*nonblank == ' ')) nonblank++;
  ds->length -= nonblank - ds->string;
  memmove (ds->string, nonblank, ds->length + 1);

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_right_adjust (dynstring *ds)
     /*
       Right-adjust the dynstring; remove the last blanks.
     */
{
  register char *pos;

  /* pre */
  assert (ds);
  assert (ds_valid_state (ds));

  pos = ds->string + ds->length - 1;
  while (pos >= ds->string) {
    if (isspace (*pos)) {
      pos--;
    } else {
      pos[1] = '\0';
      ds->length = pos - ds->string + 1;

      assert (ds_valid_state (ds));
      return;
    }
  }
  ds->string[0] = '\0';
  ds->length = 0;

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_reset (dynstring *ds)
     /*
       Reset the dynstring; make it empty.
     */
{
  assert (ds);
  assert (ds_valid_state (ds));

  ds->string[0] = '\0';
  ds->length = 0;

  assert (ds_valid_state (ds));
}


/*------------------------------------------------------------*/
void
ds_to_upper (dynstring *ds)
     /*
       Convert the characters in the dynstring to upper case.
     */
{
  register char *ch;

  assert (ds);
  assert (ds_valid_state (ds));

  for (ch = ds->string; *ch; ch++) *ch = toupper (*ch);
}


/*------------------------------------------------------------*/
void
ds_to_lower (dynstring *ds)
     /*
       Convert the characers in the dynstring to lower case.
     */
{
  register char *ch;

  assert (ds);
  assert (ds_valid_state (ds));

  for (ch = ds->string; *ch; ch++) *ch = tolower (*ch);
}
