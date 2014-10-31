/*
   Command line argument handling.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
    18-Feb-1997  first attempts
    18-Jun-1998  mod's for hgen
*/

#include "args.h"

/* public ====================
extern int args_number;
extern char *args_command;
==================== public */

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <boolean.h>
#include <str_utils.h>


/*============================================================*/
int args_number = 0;
char *args_command;

static char **args_array;
static boolean *args_flagged = NULL;


/*------------------------------------------------------------*/
void
args_initialize (int argc, char *argv[])
{
  /* pre */
  assert (argc >= 1);

  args_command = str_clone (argv[0]);
  args_number= argc;
  args_array = argv;
  if (args_flagged) free (args_flagged);
  args_flagged = calloc (argc, sizeof (boolean));
}


/*------------------------------------------------------------*/
void
args_flag (int number)
{
  /* pre */
  assert (number >= 0);
  assert (number < args_number);

  args_flagged[number] = TRUE;
}

/*------------------------------------------------------------*/
int
args_unflagged (void)
{
  int slot;

  for (slot = 0; slot < args_number; slot++) {
    if (!args_flagged[slot]) return slot;
  }

  return -1;
}


/*------------------------------------------------------------*/
int
args_exists (char *arg)
{
  int slot;

  /* pre */
  assert (arg);
  assert (*arg);

  for (slot = 1; slot < args_number; slot++) {
    if (str_eq (arg, args_array[slot])) return slot;
  }

  return 0;
}


/*------------------------------------------------------------*/
char *
args_item (int number)
{
  /* pre */
  assert (number >= 0);

  if (number >= args_number) {
    return NULL;
  } else {
    return args_array[number];
  }
}
