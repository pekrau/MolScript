/* image.h

   MolScript v2.1.2

   Image file output definitions and routines.

   This implementation requires OpenGL, GLX (X windowing system) and
   the GLUT library. It relies on the 'opengl.c' code.

   Copyright (C) 1997-1998 Per Kraulis
    11-Sep-1997  split out of jpeg.h
*/

#ifndef IMAGE_H
#define IMAGE_H 1

void image_first_plot (void);
void image_render (void);
void image_close (void);

#endif
