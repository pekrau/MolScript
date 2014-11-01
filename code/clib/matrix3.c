/*
   3D transformation matrix definition and routines.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
    27-Dec-1996  first attempts
    29-Jan-1997  fairly finished
    17-Jun-1998  mod's for hgen
*/

#include "matrix3.h"

/* public ====================
#include <vector3.h>
==================== public */

#include <assert.h>
#include <string.h>
#include <math.h>


/*------------------------------------------------------------*/
void
matrix3_initialize (double m[4][4])
     /*
       Initialize to the unit matrix.
     */
{
  /* pre */
  assert (m);

  m[0][0] = 1.0;
  m[0][1] = 0.0;
  m[0][2] = 0.0;
  m[0][3] = 0.0;
  m[1][0] = 0.0;
  m[1][1] = 1.0;
  m[1][2] = 0.0;
  m[1][3] = 0.0;
  m[2][0] = 0.0;
  m[2][1] = 0.0;
  m[2][2] = 1.0;
  m[2][3] = 0.0;
  m[3][0] = 0.0;
  m[3][1] = 0.0;
  m[3][2] = 0.0;
  m[3][3] = 1.0;
}


/*------------------------------------------------------------*/
void
matrix3_copy (double dest[4][4], double src[4][4])
{
  /* pre */
  assert (dest);
  assert (src);

  memcpy (dest, src, 16 * sizeof (double));
}


/*------------------------------------------------------------*/
void
matrix3_concatenate (double dest[4][4], double src[4][4])
{
  register int i, j;
  double t[4][4];

  /* pre */
  assert (dest);
  assert (src);

  memcpy (t, dest, 16 * sizeof (double));

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      dest[i][j] = t[i][0] * src[0][j] +
	           t[i][1] * src[1][j] +
	           t[i][2] * src[2][j] +
	           t[i][3] * src[3][j];
    }
  }
}


/*------------------------------------------------------------*/
void
matrix3_x_rotation (double m[4][4], double radians)
{
  register double d;

  /* pre */
  assert (m);

  matrix3_initialize (m);

  d = cos (radians);
  m[1][1] = d;
  m[2][2] = d;
  d = sin (radians);
  m[1][2] = d;
  m[2][1] = -d;
}


/*------------------------------------------------------------*/
void
matrix3_y_rotation (double m[4][4], double radians)
{
  register double d;

  /* pre */
  assert (m);

  matrix3_initialize (m);

  d = cos (radians);
  m[0][0] = d;
  m[2][2] = d;
  d = sin (radians);
  m[2][0] = d;
  m[0][2] = -d;
}


/*------------------------------------------------------------*/
void
matrix3_z_rotation (double m[4][4], double radians)
{
  register double d;

  /* pre */
  assert (m);

  matrix3_initialize (m);

  d = cos (radians);
  m[0][0] = d;
  m[1][1] = d;
  d = sin (radians);
  m[0][1] = d;
  m[1][0] = -d;
}


/*------------------------------------------------------------*/
void
matrix3_translation (double m[4][4], double x, double y, double z)
{
  assert (m);

  /* pre */
  matrix3_initialize (m);

  m[3][0] = x;
  m[3][1] = y;
  m[3][2] = z;
}


/*------------------------------------------------------------*/
void
matrix3_scale (double m[4][4], double sx, double sy, double sz)
{
  assert (m);

  /* pre */
  matrix3_initialize (m);

  m[0][0] = sx;
  m[1][1] = sy;
  m[2][2] = sz;
}


/*------------------------------------------------------------*/
void
matrix3_orthographic_projection (double m[4][4],
				 double left, double right,
				 double bottom, double top,
				 double near, double far)
{
  /* pre */
  assert (m);
  assert (left < right);
  assert (bottom < top);
  assert (near < far);

  matrix3_initialize (m);

  m[0][0] = 2.0 / (right - left);
  m[3][0] = - (right + left) / (right - left);
  m[1][1] = 2.0 / (top - bottom);
  m[3][1] = - (top + bottom) / (top - bottom);
  m[2][2] = -2.0 / (far - near);
  m[3][2] = - (far + near) / (far - near);
}


/*------------------------------------------------------------*/
void
matrix3_transform (vector3 *v, double m[4][4])
{
  register double x, y, z, w;

  /* pre */
  assert (v);
  assert (m);

  x = v->x;
  y = v->y;
  z = v->z;
  w = x * m[0][3] + y * m[1][3] + z * m[2][3] + m[3][3];

  v->x = (x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0]) / w;
  v->y = (x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1]) / w;
  v->z = (x * m[0][2] + y * m[1][2] + z * m[2][2] + m[3][2]) / w;
}


/*------------------------------------------------------------*/
void
matrix3_rotate (vector3 *v, double m[4][4])
{
  register double x, y, z;

  /* pre */
  assert (v);
  assert (m);

  x = v->x;
  y = v->y;
  z = v->z;

  v->x = x * m[0][0] + y * m[1][0] + z * m[2][0];
  v->y = x * m[0][1] + y * m[1][1] + z * m[2][1];
  v->z = x * m[0][2] + y * m[1][2] + z * m[2][2];
}


/*------------------------------------------------------------*/
void
matrix3_rotate_translate (vector3 *v, double m[4][4])
{
  register double x, y, z;

  /* pre */
  assert (v);
  assert (m);

  x = v->x;
  y = v->y;
  z = v->z;

  v->x = x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0];
  v->y = x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1];
  v->z = x * m[0][2] + y * m[1][2] + z * m[2][2] + m[3][2];
}
