/*
  Text file output indentation and buffering.

  clib v1.1

  Copyright (C) 1998 Per Kraulis
   23-Aug-1998  taken out of vrml.c
*/

#include "indent.h"

/* public ====================
#include <stdio.h>

#include <boolean.h>

#define INDENT_FPRINTF indent_buflen+=fprintf

extern FILE *indent_file;
extern boolean indent_flag;
extern int indent_level;
extern int indent_increment;
extern boolean indent_needs_blank;
extern int indent_buflen;
extern int indent_max_buflen;
==================== public */

#include <assert.h>
#include <ctype.h>
#include <string.h>


/*============================================================*/
FILE *indent_file = NULL;
boolean indent_flag;
int indent_level;
int indent_increment;
boolean indent_needs_blank;
int indent_buflen;
int indent_max_buflen;


/*------------------------------------------------------------*/
void
indent_initialize (FILE *file)
     /*
       Set the output file, switch on indenting, set level to 0,
       increment to 2, max_buflen to 70.
     */
{
  /* pre */
  assert (file);

  indent_file = file;
  indent_flag = TRUE;
  indent_level = 0;
  indent_increment = 2;
  indent_needs_blank = FALSE;
  indent_buflen = 0;
  indent_max_buflen = 70;
}


/*------------------------------------------------------------*/
boolean
indent_valid_state (void)
     /*
       Are the indent variables in a valid state?
     */
{
  if (indent_file == NULL) return FALSE;
  if (indent_level < 0) return FALSE;
  if (indent_increment <= 0) return FALSE;
  if (indent_max_buflen <= 0) return FALSE;
  return TRUE;
}


/*------------------------------------------------------------*/
static void
begin_line (void)
{
  assert (indent_valid_state());

  if (indent_flag && indent_buflen == 0) {
    while (indent_buflen < indent_level) indent_putchar (' ');
  }
}


/*------------------------------------------------------------*/
void
indent_putchar (char c)
{
  /* pre */
  assert (indent_valid_state());

  fputc (c, indent_file);
  indent_buflen++;
}


/*------------------------------------------------------------*/
void
indent_string (char *str)
     /*
       Output the string. Output a leading blank if needed, and set
       the flag for trailing blank.
     */
{
  int len;

  /* pre */
  assert (indent_valid_state());
  assert (str);
  assert (*str);

  len = strlen (str);
  indent_check_buflen (len);
  indent_blank();
  INDENT_FPRINTF (indent_file, "%s", str);
  indent_needs_blank = isalnum (str [len-1]);
}


/*------------------------------------------------------------*/
void
indent_blank (void)
     /*
       Output a blank character if it is needed.
     */
{
  /* pre */
  assert (indent_valid_state());

  if (indent_needs_blank) {
    indent_putchar (' ');
    indent_needs_blank = FALSE;
  }
}


/*------------------------------------------------------------*/
void
indent_newline (void)
     /*
       Output a newline to the file, and indent.
     */
{
  /* pre */
  assert (indent_valid_state());

  if (indent_buflen > 0) {
    fputc ('\n', indent_file);
    indent_buflen = 0;
  }
  indent_needs_blank = FALSE;
  begin_line();
}


/*------------------------------------------------------------*/
void
indent_newline_conditional (void)
     /*
       Output a newline if the buffer is longer than indent_level.
     */
{
  /* pre */
  assert (indent_valid_state());

  if (indent_flag && (indent_buflen != indent_level)) {
    fputc ('\n', indent_file);
    indent_buflen = 0;
    indent_needs_blank = FALSE;
    begin_line();
  }
}


/*------------------------------------------------------------*/
void
indent_check_buflen (int additional)
     /*
       Output a newline if the buffer is full (including additional)
       and indentation is in effect.
     */
{
  /* pre */
  assert (indent_valid_state());
  assert (additional >= 0);

  if (indent_flag &&
      (indent_buflen + additional >= indent_max_buflen)) indent_newline();
}


/*------------------------------------------------------------*/
void
indent_set_indent (boolean on)
     /*
       Set the indent flag.
     */
{
  /* pre */
  assert (indent_valid_state());

  indent_flag = on;
  indent_check_buflen (0);
}


/*------------------------------------------------------------*/
void
indent_set_level (int new)
     /*
       Set the indent level.
     */
{
  /* pre */
  assert (indent_valid_state());
  assert (new >= 0);

  indent_level = new;
}


/*------------------------------------------------------------*/
void
indent_set_max_buflen (int new)
     /*
       Set the output buffer max length.
     */
{
  /* pre */
  assert (indent_valid_state());
  assert (new > 0);

  indent_max_buflen = new;
  indent_check_buflen (0);
}


/*------------------------------------------------------------*/
void
indent_increment_level (boolean incr)
     /*
       Increment the indent level, or decrement.
     */
{
  /* pre */
  assert (indent_valid_state());

  indent_check_buflen (0);
  if (incr) {
    indent_level += indent_increment;
  } else {
    indent_level -= indent_increment;
  }
  indent_needs_blank = TRUE;	/* for safety */
}
