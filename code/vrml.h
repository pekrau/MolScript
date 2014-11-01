/* vrml.h

   MolScript v2.1.2

   VRML V2.0.

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
    31-Mar-1997  fairly finished
    18-Aug-1997  split out and generalized segment handling
    15-Oct-1997  USE/DEF for helix and strand different surfaces
*/

#ifndef MOLSCRIPT_VRML_H
#define MOLSCRIPT_VRML_H 1

#include "col.h"
#include "coord.h"

void vrml_set (void);

void vrml_first_plot (void);
void vrml_finish_output (void);
void vrml_start_plot (void);
void vrml_finish_plot (void);

void vrml_set_area (void);
void vrml_set_background (void);
void vrml_anchor_start (char *str);
void vrml_anchor_description (char *str);
void vrml_anchor_parameter (char *str);
void vrml_anchor_start_geometry (void);
void vrml_anchor_finish (void);
void vrml_lod_start (void);
void vrml_lod_finish (void);
void vrml_lod_start_group (void);
void vrml_lod_finish_group (void);
void vrml_viewpoint_start (char *str);
void vrml_viewpoint_output (void);
void vrml_ms_directionallight (void);
void vrml_ms_pointlight (void);
void vrml_ms_spotlight (void);

void vrml_coil (void);
void vrml_cylinder (vector3 *v1, vector3 *v2);
void vrml_helix (void);
void vrml_label (vector3 *p, char *label, colour *c);
void vrml_line (boolean polylines);
void vrml_points (int colours);
void vrml_sphere (at3d *at, double radius);
void vrml_stick (vector3 *v1, vector3 *v2, double r1, double r2, colour *c);
void vrml_strand (void);

void vrml_object (int code, vector3 *triplets, int count);

#endif
