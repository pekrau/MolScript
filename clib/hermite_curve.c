/*
   Hermite parametric cubic 3D curve routines.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
     2-Feb-1997  fairly finished
    28-Oct-1997  added tangent routine
    17-Jun-1998  mod's for hgen
*/

#include "hermite_curve.h"

/* public ====================
#include <vector3.h>
==================== public */

#include <assert.h>


/*============================================================*/
static vector3 p1;
static vector3 p2;
static vector3 v1;
static vector3 v2;


/*------------------------------------------------------------*/
void
hermite_set (vector3 *pos_start, vector3 *pos_finish,
	     vector3 *vec_start, vector3 *vec_finish)
     /*
       Set the start and finish points and vectors of the Hermite curve.
d      */
{
  /* pre */
  assert (pos_start);
  assert (pos_finish);
  assert (vec_start);
  assert (vec_finish);

  p1 = *pos_start;
  p2 = *pos_finish;
  v1 = *vec_start;
  v2 = *vec_finish;
}


/*------------------------------------------------------------*/
void
hermite_get (vector3 *p, double t)
     /*
       Return the point on the Hermite curve corresponding to the
       given parameter value t.
     */
{
  register double t2, t3, tp1, tp2, tv1, tv2;

  /* pre */
  assert (p);
  assert (t >= 0.0);
  assert (t <= 1.0);

  t2 = t * t;
  t3 = t2 * t;
  tp1 = 2.0 * t3 - 3.0 * t2 + 1.0;
  tp2 = -2.0 * t3 + 3.0 * t2;
  tv1 = t3 - 2.0 * t2 + t;
  tv2 = t3 - t2;
  p->x = p1.x * tp1 + p2.x * tp2 + v1.x * tv1 + v2.x * tv2;
  p->y = p1.y * tp1 + p2.y * tp2 + v1.y * tv1 + v2.y * tv2;
  p->z = p1.z * tp1 + p2.z * tp2 + v1.z * tv1 + v2.z * tv2;
}


/*------------------------------------------------------------*/
void
hermite_get_tangent (vector3 *v, double t)
     /*
       Return the tangent vector of the Hermite curve corresponding
       to the given parameter value t.
     */
{
  register double t2, tp1, tp2, tv1, tv2;

  /* pre */
  assert (v);
  assert (t >= 0.0);
  assert (t <= 1.0);

  t2 = t * t;
  tp1 = 6.0 * (t2 - t);
  tp2 = 6.0 * (-t2 + t);
  tv1 = 3.0 * t2 - 4.0 * t + 1.0;
  tv2 = 3.0 * t2 - 2.0 * t;
  v->x = p1.x * tp1 + p2.x * tp2 + v1.x * tv1 + v2.x * tv2;
  v->y = p1.y * tp1 + p2.y * tp2 + v1.y * tv1 + v2.y * tv2;
  v->z = p1.z * tp1 + p2.z * tp2 + v1.z * tv1 + v2.z * tv2;
}
