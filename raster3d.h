/* raster3d.h

   MolScript v2.1.2

   Raster3D; input for the 'render' program.

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
    16-Sep-1997  label output using GLUT stroke character def's
    17-Sep-1997  fairly finished
*/

#ifndef RASTER3D_H
#define RASTER3D_H 1

#include "coord.h"

void r3d_set (void);

void r3d_first_plot (void);
void r3d_finish_output (void);
void r3d_start_plot (void);
void r3d_finish_plot (void);

void r3d_set_area (void);
void r3d_set_background (void);
void r3d_directionallight (void);
void r3d_pointlight (void);
void r3d_comment (char *str);

void r3d_coil (void);
void r3d_cylinder (vector3 *v1, vector3 *v2);
void r3d_helix (void);
void r3d_label (vector3 *p, char *label, colour *c);
void r3d_line (boolean polylines);
void r3d_points (int colours);
void r3d_sphere (at3d *at, double radius);
void r3d_stick (vector3 *v1, vector3 *v2, double r1, double r2, colour *c);
void r3d_strand (void);

void r3d_object (int code, vector3 *triplets, int count);

void r3d_set_antialiasing (int new);

#endif
