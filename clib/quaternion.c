/*
   Quaternion routines.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
    23-Jun-1997  started writing
    17-Jun-1998  mod's for hgen
*/

#include "quaternion.h"

/* public ====================
#include <vector3.h>
#include <matrix3.h>

typedef struct {
  vector3 axis;
  double phi;
} quaternion;
==================== public */

#include <assert.h>
#include <math.h>


/*------------------------------------------------------------*/
void
quat_initialize (quaternion *q, double x, double y, double z, double phi)
{
  /* pre */
  assert (q);

  q->axis.x = x;
  q->axis.y = y;
  q->axis.z = z;
  v3_normalize (&(q->axis));
  v3_scale (&(q->axis), sin (phi / 2.0));
  q->phi = cos (phi / 2.0);
}


/*------------------------------------------------------------*/
void
quat_initialize_v3 (quaternion *q, vector3 *v, double phi)
{
  /* pre */
  assert (q);
  assert (v);
  assert (v3_length (v) > 0.0);

  q->axis = *v;
  v3_normalize (&(q->axis));
  v3_scale (&(q->axis), sin (phi / 2.0));
  q->phi = cos (phi / 2.0);
}


/*------------------------------------------------------------*/
void
quat_normalize (quaternion *q)
{
  register double magnitude;

  /* pre */
  assert (q);

  magnitude = q->axis.x * q->axis.x +
              q->axis.y * q->axis.y +
              q->axis.z * q->axis.z +
              q->phi * q->phi;
  q->axis.x /= magnitude;
  q->axis.y /= magnitude;
  q->axis.z /= magnitude;
  q->phi /= magnitude;
}


/*------------------------------------------------------------*/
void
quat_add (quaternion *dest, quaternion *incr)
{
  vector3 t1, t2, t3;
  quaternion tq;

  /* pre */
  assert (dest);
  assert (incr);

  t1 = dest->axis;
  v3_scale (&t1, incr->phi);
  t2 = incr->axis;
  v3_scale (&t2, dest->phi);
  v3_cross_product (&t3, &(dest->axis), &(incr->axis));
  v3_sum (&(tq.axis), &t1, &t2);
  v3_add (&(tq.axis), &t3);
  tq.phi = incr->phi * dest->phi -
           v3_dot_product (&(incr->axis), &(dest->axis));
  *dest = tq;
}


/*------------------------------------------------------------*/
void
quat_to_matrix3 (double m[4][4], quaternion *q)
{
  /* pre */
  assert (m);
  assert (q);

  m[0][0] = 1.0 - 2.0 * (q->axis.y * q->axis.y + q->axis.z * q->axis.z);
  m[0][1] = 2.0 * (q->axis.x * q->axis.y - q->axis.z * q->phi);
  m[0][2] = 2.0 * (q->axis.z * q->axis.x + q->axis.y * q->phi);
  m[0][3] = 0.0;

  m[1][0] = 2.0 * (q->axis.x * q->axis.y + q->axis.z * q->phi);
  m[1][1] = 1.0 - 2.0 * (q->axis.z * q->axis.z + q->axis.x * q->axis.x);
  m[1][2] = 2.0 * (q->axis.y * q->axis.z - q->axis.x * q->phi);
  m[1][3] = 0.0;

  m[2][0] = 2.0 * (q->axis.z * q->axis.x - q->axis.y * q->phi);
  m[2][1] = 2.0 * (q->axis.y * q->axis.z + q->axis.x * q->phi);
  m[2][2] = 1.0 - 2.0 * (q->axis.y * q->axis.y + q->axis.x * q->axis.x);
  m[2][3] = 0.0;

  m[3][0] = 0.0;
  m[3][1] = 0.0;
  m[3][2] = 0.0;
  m[3][3] = 1.0;
}


/*------------------------------------------------------------*/
void
quat_interpolate (quaternion *dest, quaternion *q0, quaternion *q1, double t)
{
  register double sum, theta, beta1, beta2;

  /* pre */
  assert (dest);
  assert (q0);
  assert (q1);
  assert (t >= 0.0);
  assert (t <= 1.0);

  sum = q0->axis.x * q1->axis.x +
        q0->axis.y * q1->axis.y +
        q0->axis.z * q1->axis.z +
        q0->phi * q1->phi;
  theta = acos (sum);
  if (theta < 1.0e-10) {
    beta1 = 1.0 - t;
    beta2 = t;
  } else {
    beta1 = sin ((1.0 - t) * theta) / sin (theta);
    beta2 = sin (t * theta) / sin (theta);
  }

  dest->axis.x = beta1 * q0->axis.x + beta2 * q1->axis.x;
  dest->axis.y = beta1 * q0->axis.y + beta2 * q1->axis.y;
  dest->axis.z = beta1 * q0->axis.z + beta2 * q1->axis.z;
  dest->phi = beta1 * q0->phi + beta2 * q1->phi;
}


/*------------------------------------------------------------*/
static double
project_to_sphere (double r, double x, double y)
{
  register double d, t, z;

  d = sqrt (x*x + y*y);
  if (d < r * 0.70710678118654752440) { /* Inside sphere. */
    z = sqrt (r*r - d*d);
  } else {			        /* On hyperbola. */
    t = r / 1.41421356237309504880;
    z = t*t / d;
  }

  return z;
}


/*------------------------------------------------------------*/
void
quat_trackball (quaternion *dest, double size,
		double p1x, double p1y, double p2x, double p2y)
{
  vector3 p1, p2, axis;
  double t;

  /* pre */
  assert (dest);
  assert (size > 0.0);
  assert (size <= 1.0);
  assert (p1x >= -1.0);
  assert (p1x <= 1.0);
  assert (p1y >= -1.0);
  assert (p1y <= 1.0);
  assert (p2x >= -1.0);
  assert (p2x <= 1.0);
  assert (p2y >= -1.0);
  assert (p2y <= 1.0);

  if ((p1x == p2x) && (p1y == p2y)) { /* Zero rotation. */
    quat_initialize (dest, 1.0, 0.0, 0.0, 0.0);
    return;
  }

				/* Z coord for proj onto sphere. */
  v3_initialize (&p1, p1x, p1y, project_to_sphere (size, p1x, p1y));
  v3_initialize (&p2, p2x, p2y, project_to_sphere (size, p2x, p2y));

  v3_cross_product (&axis, &p2, &p1);

  t = v3_distance (&p1, &p2) / (2.0 * size); /* Rotation amount. */
  if (t > 1.0) {
    t = 1.0;
  } else if (t < -1.0) {
    t= -1.0;
  }

  quat_initialize_v3 (dest, &axis, 2.0 * asin (t));
}
