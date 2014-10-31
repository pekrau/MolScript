/* opengl.h

   MolScript v2.1.2

   OpenGL.

   This implementation requires the GLUT library.

   Copyright (C) 1997-1998 Per Kraulis
     25-Jun-1997
     11-Sep-1997  fairly finished
*/

#ifndef OPENGL_H
#define OPENGL_H 1

#include "coord.h"

void ogl_set (void);

void ogl_first_plot (void);
void ogl_start_plot (void);
void ogl_finish_plot (void);

void ogl_start_plot_general (void);
void ogl_render_init (void);
void ogl_render_lights (void);
void ogl_render_lists (void);

int ogl_accum (void);
void ogl_set_accum (int number);

void ogl_set_area (void);
void ogl_set_background (void);
void ogl_directionallight (void);
void ogl_pointlight (void);
void ogl_spotlight (void);

void ogl_coil (void);
void ogl_cylinder (vector3 *v1, vector3 *v2);
void ogl_helix (void);
void ogl_label (vector3 *p, char *label, colour *c);
void ogl_line (boolean polylines);
void ogl_sphere (at3d *at, double radius);
void ogl_stick (vector3 *v1, vector3 *v2, double r1, double r2, colour *c);
void ogl_strand (void);

void ogl_start_object (void);
void ogl_object (int code, vector3 *triplets, int count);
void ogl_finish_object (void);

void ogl_pickable (at3d *atom);

#endif
