/* xform.c

   MolScript v2.1.2

   Coordinate transformation.

   Copyright (C) 1997-1998 Per Kraulis
     6-Apr-1997  split out of coord.c
    23-Jun-1997  added axis rotation
*/

#include <assert.h>

#include "clib/angle.h"
#include "clib/matrix3.h"
#include "clib/quaternion.h"

#include "xform.h"
#include "global.h"
#include "select.h"


/*============================================================*/
double xform[4][4];

static double tmp_xform[4][4];
static double stored_xform[4][4];


/*------------------------------------------------------------*/
void
xform_init (void)
{
  matrix3_initialize (xform);
}


/*------------------------------------------------------------*/
void
xform_init_stored (void)
{
  matrix3_initialize (stored_xform);
}


/*------------------------------------------------------------*/
void
xform_store (void)
{
  matrix3_copy (stored_xform, xform);
}


/*------------------------------------------------------------*/
void
xform_centre (void)
{
  assert (dstack_size == 3);

  matrix3_translation (tmp_xform, - dstack[0], - dstack[1], - dstack[2]);
  matrix3_concatenate (xform, tmp_xform);

  clear_dstack();
}


/*------------------------------------------------------------*/
void
xform_translation (void)
{
  assert (dstack_size == 3);

  matrix3_translation (tmp_xform, dstack[0], dstack[1], dstack[2]);
  matrix3_concatenate (xform, tmp_xform);

  clear_dstack();
}


/*------------------------------------------------------------*/
void
xform_rotation_x (void)
{
  assert (dstack_size == 1);

  matrix3_x_rotation (tmp_xform, to_radians (dstack[0]));
  matrix3_concatenate (xform, tmp_xform);

  clear_dstack();
}


/*------------------------------------------------------------*/
void
xform_rotation_y (void)
{
  assert (dstack_size == 1);

  matrix3_y_rotation (tmp_xform, to_radians (dstack[0]));
  matrix3_concatenate (xform, tmp_xform);

  clear_dstack();
}


/*------------------------------------------------------------*/
void
xform_rotation_z (void)
{
  assert (dstack_size == 1);

  matrix3_z_rotation (tmp_xform, to_radians (dstack[0]));
  matrix3_concatenate (xform, tmp_xform);

  clear_dstack();
}


/*------------------------------------------------------------*/
void
xform_rotation_axis (void)
{
  vector3 v;
  quaternion q;

  assert (dstack_size == 4);

  v3_initialize (&v, dstack[0], dstack[1], dstack[2]);
  if (v3_length (&v) <= 1.0e-10) {
    yyerror ("invalid rotation axis: zero length");
    return;
  }
  quat_initialize_v3 (&q, &v, to_radians (dstack[3]));
  quat_to_matrix3 (tmp_xform, &q);
  matrix3_concatenate (xform, tmp_xform);

  clear_dstack();
}


/*------------------------------------------------------------*/
void
xform_rotation_matrix (void)
{
  assert (dstack_size == 9);

  matrix3_initialize (tmp_xform);
  tmp_xform[0][0] = dstack[0];
  tmp_xform[1][0] = dstack[1];
  tmp_xform[2][0] = dstack[2];
  tmp_xform[0][1] = dstack[3];
  tmp_xform[1][1] = dstack[4];
  tmp_xform[2][1] = dstack[5];
  tmp_xform[0][2] = dstack[6];
  tmp_xform[1][2] = dstack[7];
  tmp_xform[2][2] = dstack[8];
  matrix3_concatenate (xform, tmp_xform);

  clear_dstack();
}


/*------------------------------------------------------------*/
void
xform_recall_matrix (void)
{
  matrix3_concatenate (xform, stored_xform);
}


/*------------------------------------------------------------*/
void
xform_atoms (void)
{
  mol3d *mol;
  res3d *res;
  at3d *at;
  int *flags;
  int count = 0;
  named_data *nd;

  assert (count_atom_selections() == 1);

  flags = current_atom_sel->flags;
  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) {
	if (*flags++) {
	  matrix3_transform (&(at->xyz), xform);
	  count++;
	}
      }
    }
  }

  pop_atom_selection();

  if (message_mode) {
    int i, j;
    fprintf (stderr, "%i atoms selected for transform\n", count);
    fprintf (stderr, "rotation matrix applied:\n");
    for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++)
	fprintf (stderr, "%.8f ", xform[j][i]);
      fprintf (stderr, "\n");
    }
    fprintf (stderr, "translation vector applied:\n");
    for (i = 0; i < 3; i++)
      fprintf (stderr, "%.4f ", xform[3][i]);
    fprintf (stderr, "\n");
  }

  assert (count_atom_selections() == 0);
}
