/* png_img.h

   MolScript v2.1.2

   PNG image file.

   This implementation is based on the PNG Reference Library 1.0
   version 0.96 and zlib v1.0.4. It relies on the 'image.c' code,
   and therefore implicitly on OpenGL and GLX.

   Copyright (C) 1997-1998 Per Kraulis
    12-Sep-1997  started
*/

#ifndef PNG_IMG_H
#define PNG_IMG_H 1

void pngi_set (void);
void pngi_first_plot (void);
void pngi_finish_output (void);
int pngi_set_compression (char *level);

#endif
