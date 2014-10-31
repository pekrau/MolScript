/*
   Error message procedures. The messages are written to stderr.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
    15-May-1997  first attempts
    15-Apr-1998  hgen modifications
*/

#include "err.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <str_utils.h>


/*============================================================*/
static char *prefix = NULL;


/*------------------------------------------------------------*/
void
err_set_prefix (const char *new)
     /*
       Set the prefix (usually program name) in error messages.
     */
{
  if (prefix) free (prefix);
  if (new) {
    prefix = str_clone (new);
  } else {
    prefix = NULL;
  }
}


/*------------------------------------------------------------*/
void
err_fatal (const char *msg)
     /*
       Output error message and abort execution.
     */
{
  if (prefix) {
    if (msg) {
      fprintf (stderr, "%s error: %s\n", prefix, msg);
    } else {
      fprintf (stderr, "%s error\n", prefix);
    }
  } else {
    if (msg) {
      fprintf (stderr, "Error: %s\n", msg);
    } else {
      fprintf (stderr, "Error\n");
    }
  }
  exit (1);
}


/*------------------------------------------------------------*/
void
err_message (const char *msg)
     /*
       Output error message.
     */
{
  if (prefix) {
    if (msg) {
      fprintf (stderr, "%s error: %s\n", prefix, msg);
    } else {
      fprintf (stderr, "%s error\n", prefix);
    }
  } else {
    if (msg) {
      fprintf (stderr, "Error: %s\n", msg);
    } else {
      fprintf (stderr, "Error\n");
    }
  }
}


/*------------------------------------------------------------*/
void
err_usage (const char *msg)
     /*
       Output usage message and abort execution.
     */
{
  /* pre */
  assert (msg);

  if (prefix) {
    fprintf (stderr, "%s usage: %s\n", prefix, msg);
  } else {
    fprintf (stderr, "Usage: %s\n", msg);
  }
  exit (1);
}


/*------------------------------------------------------------*/
void
err_warning (const char *msg)
     /*
       Output warning message.
     */
{
  if (prefix) {
    if (msg) {
      fprintf (stderr, "%s warning: %s\n", prefix, msg);
    } else {
      fprintf (stderr, "%s warning\n", prefix);
    }
  } else {
    if (msg) {
      fprintf (stderr, "Warning: %s\n", msg);
    } else {
      fprintf (stderr, "Warning\n");
    }
  }
}
