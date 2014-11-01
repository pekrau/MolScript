/*
   Some OpenGL utility procedures.

   This package relies on GLUT version 3.5 (and presumably higher).

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
     6-Aug-1997  first attempts
    12-Jun-1998  reorganized, mod's for hgen
*/

#include "ogl_utils.h"

/* public ====================
#include <boolean.h>
==================== public */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <GL/glut.h>

#include <err.h>


/*------------------------------------------------------------*/
boolean
ogl_check_errors (char *msg)
     /*
       Check if any OpenGL errors have occurred, output error message
       and return TRUE if so, otherwise FALSE.
     */
{
  GLenum error = glGetError();

  if (error == GL_NO_ERROR) return FALSE;

  if (msg) fprintf (stderr, "%s", msg);
  while (error != GL_NO_ERROR) {
    fprintf (stderr, "OpenGL error: %s\n", gluErrorString (error));
    error = glGetError();
  }
  return TRUE;
}


/*------------------------------------------------------------*/
boolean
ogl_display_mode_not_available (void)
     /*
       Check if the requested GLUT window display mode is
       not available.
     */
{
  if (! glutGet (GLUT_DISPLAY_MODE_POSSIBLE)) {
    err_message ("required OpenGL display mode not available on this system");
    return TRUE;
  } else {
    return FALSE;
  }
}


/*------------------------------------------------------------*/
void
ogl_display_mode_not_available_fatal (void)
     /*
       Check if the requested GLUT window display mode is
       not available and exit with failure if so.
     */
{
  if (ogl_display_mode_not_available()) {
    fprintf (stderr, "exiting...");
    exit (EXIT_FAILURE);
  }
}
