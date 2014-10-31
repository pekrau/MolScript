/*
  3D vector routines.

  clib v1.1

  Copyright (C) 1997-1998 Per Kraulis
   27-Dec-1996  first attempts
    6-Feb-1997  fairly finished
   10-Mar-1997  fixed bug in cross product routine
   19-Feb-1998  minor mod's
   19-Mar-1998  modified for hgen
*/

#include "vector3.h"

/* public ====================
#include <boolean.h>

typedef struct {
  double x, y, z;
} vector3;
==================== public */

#include <assert.h>
#include <math.h>


/*------------------------------------------------------------*/
void
v3_initialize (vector3 *dest, const double x, const double y, const double z)
{
  /* pre */
  assert (dest);

  dest->x = x;
  dest->y = y;
  dest->z = z;
}


/*------------------------------------------------------------*/
void
v3_sum (vector3 *dest, const vector3 *v1, const vector3 *v2)
     /*
       dest = v1 + v2
     */
{
  /* pre */
  assert (dest);
  assert (v1);
  assert (v2);

  dest->x = v1->x + v2->x;
  dest->y = v1->y + v2->y;
  dest->z = v1->z + v2->z;
}


/*------------------------------------------------------------*/
void
v3_difference (vector3 *dest, const vector3 *v1, const vector3 *v2)
     /*
       dest = v1 - v2
     */
{
  /* pre */
  assert (dest);
  assert (v1);
  assert (v2);

  dest->x = v1->x - v2->x;
  dest->y = v1->y - v2->y;
  dest->z = v1->z - v2->z;
}


/*------------------------------------------------------------*/
void
v3_scaled (vector3 *dest, const double s, const vector3 *src)
     /*
       dest = s * src
     */
{
  /* pre */
  assert (dest);
  assert (src);

  dest->x = s * src->x;
  dest->y = s * src->y;
  dest->z = s * src->z;
}


/*------------------------------------------------------------*/
void
v3_add (vector3 *dest, const vector3 *v)
     /*
       dest += v
     */
{
  /* pre */
  assert (dest);
  assert (v);

  dest->x += v->x;
  dest->y += v->y;
  dest->z += v->z;
}


/*------------------------------------------------------------*/
void
v3_subtract (vector3 *dest, const vector3 *v)
     /*
       dest -= v
     */
{
  /* pre */
  assert (dest);
  assert (v);

  dest->x -= v->x;
  dest->y -= v->y;
  dest->z -= v->z;
}


/*------------------------------------------------------------*/
void
v3_scale (vector3 *dest, const double s)
     /*
       dest *= s
     */
{
  /* pre */
  assert (dest);

  dest->x *= s;
  dest->y *= s;
  dest->z *= s;
}


/*------------------------------------------------------------*/
void
v3_invert (vector3 *v)
     /*
       v = (1/x, 1/y, 1/z)
     */
{
  /* pre */
  assert (v);
  assert (v->x != 0.0);
  assert (v->y != 0.0);
  assert (v->z != 0.0);

  v->x = 1.0 / v->x;
  v->y = 1.0 / v->y;
  v->z = 1.0 / v->z;
}


/*------------------------------------------------------------*/
void
v3_reverse (vector3 *v)
     /*
       v = -v
     */
{
  /* pre */
  assert (v);

  v->x = - v->x;
  v->y = - v->y;
  v->z = - v->z;
}


/*------------------------------------------------------------*/
void
v3_sum_scaled (vector3 *dest, const vector3 *v1,
	       const double s, const vector3 *v2)
     /*
       dest = v1 + s * v2
     */
{
  /* pre */
  assert (dest);
  assert (v1);
  assert (v2);

  dest->x = v1->x + s * v2->x;
  dest->y = v1->y + s * v2->y;
  dest->z = v1->z + s * v2->z;
}


/*------------------------------------------------------------*/
void
v3_add_scaled (vector3 *dest, const double s, const vector3 *v)
     /*
       dest += s * v
     */
{
  /* pre */
  assert (dest);
  assert (v);

  dest->x += s * v->x;
  dest->y += s * v->y;
  dest->z += s * v->z;
}


/*------------------------------------------------------------*/
double
v3_length (const vector3 *v)
{
  /* pre */
  assert (v);

  return sqrt (v->x * v->x + v->y * v->y + v->z * v->z);
}


/*------------------------------------------------------------*/
double
v3_angle (const vector3 *v1, const vector3 *v2)
     /*
       The angle (in radians) between the vectors v1 and v2.
     */
{
  /* pre */
  assert (v1);
  assert (v2);
  assert (v3_length (v1) > 0.0);
  assert (v3_length (v2) > 0.0);

  return acos ((v1->x * v2->x + v1->y * v2->y + v1->z * v2->z) /
	       sqrt ((v1->x * v1->x + v1->y * v1->y + v1->z * v1->z) *
		     (v2->x * v2->x + v2->y * v2->y + v2->z * v2->z)));
}


/*------------------------------------------------------------*/
double
v3_angle_points (const vector3 *p1, const vector3 *p2, const vector3 *p3)
     /*
       The angle (in radians) formed between the vector from p2 to p1
       and the vector from p2 to p3.
     */
{
  vector3 v1, v2;

  /* pre */
  assert (p1);
  assert (p2);
  assert (p3);

  v3_difference (&v1, p1, p2);
  v3_difference (&v2, p3, p2);
  return v3_angle (&v1, &v2);
}


/*------------------------------------------------------------*/
double
v3_torsion (const vector3 *p1, const vector3 *v1,
	    const vector3 *p2, const vector3 *v2)
     /*
       The torsion angle (in radians) between the vectors v1 and v2
       when viewed along the vector from p1 to p2.
     */
{
  double angle;
  vector3 dir, x1, x2;

  /* pre */
  assert (p1);
  assert (v1);
  assert (p2);
  assert (v2);

  v3_difference (&dir, p2, p1);
  v3_cross_product (&x1, &dir, v1);
  v3_cross_product (&x2, &dir, v2);
  angle = v3_angle (&x1, &x2);
  return (v3_dot_product (v1, &x2) > 0.0) ? angle : -angle;
}


/*------------------------------------------------------------*/
double
v3_torsion_points (const vector3 *p1, const vector3 *p2,
		   const vector3 *p3, const vector3 *p4)
     /*
       The torsion angle (in radians) formed between the points.
     */
{
  vector3 v1, v2;

  /* pre */
  assert (p1);
  assert (p2);
  assert (p3);
  assert (p4);
  assert (p1 != p2);
  assert (p2 != p3);
  assert (p3 != p4);

  v3_difference (&v1, p1, p2);
  v3_difference (&v2, p4, p3);
  return v3_torsion (p2, &v1, p3, &v2);
}


/*------------------------------------------------------------*/
double
v3_dot_product (const vector3 *v1, const vector3 *v2)
{
  /* pre */
  assert (v1);
  assert (v2);

  return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}


/*------------------------------------------------------------*/
void
v3_cross_product (vector3 *vz, const vector3 *vx, const vector3 *vy)
{
  /* pre */
  assert (vz);
  assert (vx);
  assert (vy);
  assert (vz != vx);
  assert (vz != vy);

  vz->x = vx->y * vy->z - vx->z * vy->y;
  vz->y = vx->z * vy->x - vx->x * vy->z;
  vz->z = vx->x * vy->y - vx->y * vy->x;
}


/*------------------------------------------------------------*/
void
v3_triangle_normal (vector3 *n,
		    const vector3 *p1, const vector3 *p2, const vector3 *p3)
     /*
       The normal vector n for the plane defined by the three points.
     */
{
  vector3 p12, p23;

  /* pre */
  assert (n);
  assert (p1);
  assert (p2);
  assert (p3);
  assert (v3_distance (p1, p2) > 0.0);
  assert (v3_distance (p2, p3) > 0.0);
  assert (v3_distance (p3, p1) > 0.0);

  v3_difference (&p12, p2, p1);
  v3_difference (&p23, p3, p2);
  v3_cross_product (n, &p12, &p23);
  v3_normalize (n);
}


/*------------------------------------------------------------*/
double
v3_sqdistance (const vector3 *p1, const vector3 *p2)
     /*
       The squared distance between the points.
     */
{
  register double xdiff, ydiff, zdiff;

  /* pre */
  assert (p1);
  assert (p2);

  xdiff = p1->x - p2->x;
  ydiff = p1->y - p2->y;
  zdiff = p1->z - p2->z;

  return xdiff * xdiff + ydiff * ydiff + zdiff * zdiff;
}


/*------------------------------------------------------------*/
double
v3_distance (const vector3 *p1, const vector3 *p2)
{
  register double xdiff, ydiff, zdiff;

  /* pre */
  assert (p1);
  assert (p2);

  xdiff = p1->x - p2->x;
  ydiff = p1->y - p2->y;
  zdiff = p1->z - p2->z;

  return sqrt (xdiff * xdiff + ydiff * ydiff + zdiff * zdiff);
}


/*------------------------------------------------------------*/
boolean
v3_close (const vector3 *v1, const vector3 *v2, const double sqdistance)
     /*
       Are the two points closer than the given squared distance?
     */
{
  register double xdiff;

  /* pre */
  assert (v1);
  assert (v2);

  xdiff = v1->x - v2->x;
  xdiff *= xdiff;
  if (xdiff > sqdistance) {
    return FALSE;
  } else {
    register double ydiff = v1->y - v2->y;
    ydiff *= ydiff;
    if (ydiff > sqdistance) {
      return FALSE;
    } else {
      register double zdiff = v1->z - v2->z;
      return (xdiff + ydiff + zdiff * zdiff) <= sqdistance;
    }
  }
}


/*------------------------------------------------------------*/
void
v3_normalize (vector3 *v)
     /*
       v /= length (v)
    */
{
  register double ir;

  /* pre */
  assert (v);
  assert (v3_length (v) > 0.0);

  ir = 1.0 / sqrt (v->x * v->x + v->y * v->y + v->z * v->z);
  v->x *= ir;
  v->y *= ir;
  v->z *= ir;
}


/*------------------------------------------------------------*/
void
v3_middle (vector3 *dest, const vector3 *p1, const vector3 *p2)
     /*
       dest = (p1 + p2) / 2
     */
{
  /* pre */
  assert (dest);
  assert (p1);
  assert (p2);

  dest->x = 0.5 * (p1->x + p2->x);
  dest->y = 0.5 * (p1->y + p2->y);
  dest->z = 0.5 * (p1->z + p2->z);
}


/*------------------------------------------------------------*/
void
v3_between (vector3 *dest,
	    const vector3 *p1, const vector3 *p2, const double fraction)
     /*
       dest = fraction * (p2 - p1) + p1
     */
{
  /* pre */
  assert (dest);
  assert (p1);
  assert (p2);

  dest->x = fraction * (p2->x - p1->x) + p1->x;
  dest->y = fraction * (p2->y - p1->y) + p1->y;
  dest->z = fraction * (p2->z - p1->z) + p1->z;
}


/*------------------------------------------------------------*/
int
v3_different (const vector3 *v1, const vector3 *v2)
{
  /* pre */
  assert (v1);
  assert (v2);

  return ((v1->x != v2->x) || (v1->y != v2->y) || (v1->z != v2->z));
}
