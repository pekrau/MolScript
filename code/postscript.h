/* postscript.h

   MolScript v2.1.2

   PostScript file.

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
*/

#ifndef POSTSCRIPT_H
#define POSTSCRIPT_H 1

#include "coord.h"

void ps_set (void);

void ps_first_plot (void);
void ps_finish_output (void);
void ps_start_plot (void);
void ps_finish_plot (void);

void ps_set_eps (void);
void ps_set_background (void);
void ps_set_area (void);

void ps_coil (void);
void ps_cylinder (vector3 *v1, vector3 *v2);
void ps_helix (void);
void ps_label (vector3 *p, char *label, colour *c);
void ps_line (boolean polylines);
void ps_stick (vector3 *v1, vector3 *v2, double r1, double r2, colour *c);
void ps_sphere (at3d *at, double radius);
void ps_strand (void);

void ps_object (int code, vector3 *triplets, int count);

#endif
