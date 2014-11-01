/* io_utils

   File input/output utility procedures.

   clib v1.1

   Copyright (C) 1998 Per Kraulis
     2-Mar-1998  first attempts
     4-May-1998  modified for hgen, name changes
    27-May-1998  added dynstring input
*/

#include "io_utils.h"

/* public ====================
#include <stdio.h>

#include <boolean.h>
#include <dynstring.h>
==================== public */

#include <assert.h>


/*------------------------------------------------------------*/
boolean
io_fprint_str (FILE *file, const char *str)
     /*
       Write the string to the opened file. Return TRUE if successful.
     */
{
  /* pre */
  assert (file);
  assert (str);

  return (fprintf (file, "%s", str) > 0);
}


/*------------------------------------------------------------*/
boolean
io_fprint_str_length (FILE *file, const char *str, int length)
     /*
       Write the string to the opened file. The number of output characters
       is given by the length; blanks are used to pad if the string is not
       sufficiently long. Return TRUE if successful.
     */
{
  /* pre */
  assert (file);
  assert (length >= 0);

  if (str) {
    while ((length-- > 0) && *str) {
      if (fputc (*str++, file) == EOF) return FALSE;
    }
  }

  while (length-- > 0) {
    if (fputc (' ', file) == EOF) return FALSE;
  }

  return TRUE;
}


/*------------------------------------------------------------*/
boolean
io_fprint_blanks (FILE *file, int count)
     /*
       Write the given number of blank characters to the file.
       Return TRUE if successful.
     */
{
  /* pre */
  assert (file);

  while (count > 0) {
    if (fputc (' ', file) == EOF) return FALSE;
    count--;
  }

  return TRUE;
}


/*------------------------------------------------------------*/
boolean
io_fget_ds (FILE *file, dynstring *ds)
     /*
       Read one complete line (up to, but excluding, the '\n' character).
       Return TRUE if successful.
     */
{
  int ch;

  /* pre */
  assert (file);
  assert (ds);
  assert (ds_valid_state (ds));

  ds_reset (ds);
  while (ch = fgetc (file)) {
    if (ch == '\n') break;
    if (ch == EOF) break;
    ds_add (ds, ch);
  }

  return TRUE;
}
