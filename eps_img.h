/* eps_img.h

   MolScript v2.1.2

   Encapsulated PostScript (EPS) image file.

   This implementation relies on the 'image.c' code, and therefore
   implicitly on OpenGL and GLX.

   Copyright (C) 1997-1998 Per Kraulis
    13-Sep-1997  working
*/

#ifndef EPS_IMG_H
#define EPS_IMG_H 1

void eps_set (void);
void eps_first_plot (void);
void eps_finish_output (void);
void eps_set_bw (void);
void eps_set_scale (float new);

#endif
