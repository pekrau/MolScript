/* jpeg_img.h

   MolScript v2.1.2

   JPEG image file.

   This implementation is based on the Independent JPEG Group's (IJG)
   JPEG library (release 6a). It relies on the 'image.c' code, and
   therefore implicitly on OpenGL and GLX.

   Copyright (C) 1997-1998 Per Kraulis
     9-Sep-1997  started
    11-Sep-1997  working
*/

#ifndef JPEG_IMG_H
#define JPEG_IMG_H 1

void jpgi_set (void);
void jpgi_first_plot (void);
void jpgi_finish_output (void);
void jpgi_set_quality (int new);

#endif
