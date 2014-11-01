/* xform.h

   MolScript v2.1.2

   Coordinate transformation.

   Copyright (C) 1997-1998 Per Kraulis
     6-Apr-1997  split out of coord.h
    23-Jun-1997  added axis rotation
*/

#ifndef XFORM_H
#define XFORM_H 1

#include "clib/mol3d.h"

extern double xform[4][4];

void xform_init (void);
void xform_init_stored (void);
void xform_store (void);

void xform_centre (void);
void xform_translation (void);
void xform_rotation_x (void);
void xform_rotation_y (void);
void xform_rotation_z (void);
void xform_rotation_axis (void);
void xform_rotation_matrix (void);
void xform_recall_matrix (void);

void xform_atoms (void);

#endif
