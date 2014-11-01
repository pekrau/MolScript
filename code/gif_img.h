/* gif_img.h

   MolScript v2.1.2

   GIF image file.

   This implementation uses the gd 1.3 library by Thomas Boutell,
   http://www.boutell.com. The source code for this library does *not*
   use LZW compression, so there is no conflict with the (infamous)
   Unisys patent on the LZW algorithm. This implementation relies on
   the 'image.c' code, and therefore implicitly on OpenGL and GLX.

   Copyright (C) 1998 Per Kraulis
    29-Jul-1998  first attempts
*/

#ifndef GIF_IMG_H
#define GIF_IMG_H 1

void gifi_set (void);
void gifi_first_plot (void);
void gifi_finish_output (void);

#endif
