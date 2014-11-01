/*
   Various 3D bodies: data and routines.

   clib v1.1

   Copyright (C) 1998 Per Kraulis
    10-Jun-1998  first attempts, generalized from other code
*/

#include "body3d.h"

/* public ====================
#include <vector3.h>
==================== public */

#include <assert.h>
#include <stdlib.h>
#include <math.h>


/*============================================================*/
static vector3 *icosahedron = NULL;
static vector3 *current;
static int count;
static int parts;
static double fraction;


/*------------------------------------------------------------*/
static void
sphere_triangles (vector3 *ico1, vector3 *ico2, vector3 *ico3)
{
  int slot;
  vector3 p1, p2;

  assert (ico1);
  assert (ico2);
  assert (ico3);
  assert (fraction >= 0.0);
  assert (fraction <= 1.0);
  assert (parts > 0);

  v3_between (&p1, ico1, ico3, fraction);
  v3_normalize (&p1);
  v3_between (&p2, ico2, ico3, fraction);
  v3_normalize (&p2);
  for (slot = 1; slot <= parts; slot++) {
    v3_between (current + count, &p1, &p2, (double) slot / (double) parts);
    v3_normalize (current + count);
    count++;
  }
}


/*------------------------------------------------------------*/
vector3 *
sphere_ico_points (int level)
     /*
       Return an array of points on the unit sphere, using the
       icosahedron algorithm. The level determines the number of points.
     */
{
  vector3 *new;
  int vertex;

  /* pre */
  assert (level >= 1);

  if (icosahedron == NULL) icosahedron = icosahedron_vertices();

  new = malloc (sphere_ico_point_count (level) * sizeof (vector3));
  current = new;
  count = 0;

  *(current + count++) = *icosahedron;
  *(current + count++) = *(icosahedron + 9);

  for (vertex = 0; vertex < level; vertex++) {
    fraction = (double) vertex / (double) level;
    parts = level - vertex;
    sphere_triangles (icosahedron + 1, icosahedron + 4, icosahedron + 0);
    sphere_triangles (icosahedron + 4, icosahedron + 8, icosahedron + 0);
    sphere_triangles (icosahedron + 8, icosahedron + 3, icosahedron + 0);
    sphere_triangles (icosahedron + 3, icosahedron + 2, icosahedron + 0);
    sphere_triangles (icosahedron + 2, icosahedron + 1, icosahedron + 0);
    sphere_triangles (icosahedron + 5, icosahedron + 7, icosahedron + 9);
    sphere_triangles (icosahedron + 7, icosahedron + 10, icosahedron + 9);
    sphere_triangles (icosahedron + 10, icosahedron + 11, icosahedron + 9);
    sphere_triangles (icosahedron + 11, icosahedron + 6, icosahedron + 9);
    sphere_triangles (icosahedron + 6, icosahedron + 5, icosahedron + 9);
  }

  for (vertex = 1; vertex < level; vertex++) {
    fraction = (double) vertex / (double) level;
    parts = level - vertex;
    sphere_triangles (icosahedron + 2, icosahedron + 1, icosahedron + 5);
    sphere_triangles (icosahedron + 5, icosahedron + 6, icosahedron + 1);
    sphere_triangles (icosahedron + 1, icosahedron + 4, icosahedron + 6);
    sphere_triangles (icosahedron + 6, icosahedron + 11, icosahedron + 4);
    sphere_triangles (icosahedron + 4, icosahedron + 8, icosahedron + 11);
    sphere_triangles (icosahedron + 11, icosahedron + 10, icosahedron + 8);
    sphere_triangles (icosahedron + 8, icosahedron + 3, icosahedron + 10);
    sphere_triangles (icosahedron + 10, icosahedron + 7, icosahedron + 3);
    sphere_triangles (icosahedron + 3, icosahedron + 2, icosahedron + 7);
    sphere_triangles (icosahedron + 7, icosahedron + 5, icosahedron + 2);
  }

  assert (count == sphere_ico_point_count (level));

  return new;
}


/*------------------------------------------------------------*/
int
sphere_ico_point_count (int level)
     /*
       Return the number of points generated for the sphere given
       the level, using the icosahedron algorithm.
     */
{
  int i, count;

  /* pre */
  assert (level >= 1);

  count = 12;			/* vertices */
  count += 30 * (level - 1);	/* edges */
  for (i = 1; i < level - 1; i++) count += 20 * i; /* faces */

  return count;
}


/*------------------------------------------------------------*/
vector3 *
tetrahedron_vertices (void)
     /*
       Return a new array containing the 4 vertices of a tetrahedron.
       The vertices are on the unit sphere. The order of the
       vertices are essential for other dependent routines.
     */
{
  double a = 1.0 / sqrt (3.0);
  vector3 *v = malloc (4 * sizeof (vector3));

  v3_initialize (v, a, a, a);
  v3_initialize (v + 1, a, -a, -a);
  v3_initialize (v + 2, -a, a, -a);
  v3_initialize (v + 3, -a, -a, a);

  return v;
}


/*------------------------------------------------------------*/
vector3 *
octahedron_vertices (void)
     /*
       Return a new array containing the 6 vertices of an octahedron.
       The vertices are on the unit sphere. The order of the
       vertices are essential for other dependent routines.
     */
{
  vector3 *v = malloc (6 * sizeof (vector3));

  v3_initialize (v, 0.0, 0.0, 1.0);
  v3_initialize (v + 1, 1.0, 0.0, 0.0);
  v3_initialize (v + 2, 0.0, 1.0, 0.0);
  v3_initialize (v + 3, -1.0, 0.0, 0.0);
  v3_initialize (v + 4, 0.0, -1.0, 0.0);
  v3_initialize (v + 5, 0.0, 0.0, -1.0);

  return v;
}


/*------------------------------------------------------------*/
vector3 *
cube_vertices (void)
     /*
       Return a new array containing the 8 vertices of an octahedron.
       The vertices are on the unit sphere. The order of the
       vertices are essential for other dependent routines.
     */
{
  double a = 1.0 / sqrt (3.0);
  vector3 *v = malloc (8 * sizeof (vector3));

  v3_initialize (v, a, a, a);
  v3_initialize (v + 1, a, a, -a);
  v3_initialize (v + 2, -a, a, a);
  v3_initialize (v + 3, -a, a, -a);
  v3_initialize (v + 4, -a, -a, a);
  v3_initialize (v + 5, -a, -a, -a);
  v3_initialize (v + 6, a, -a, a);
  v3_initialize (v + 7, a, -a, -a);

  return v;
}


/*------------------------------------------------------------*/
vector3 *
icosahedron_vertices (void)
     /*
       Return a new array containing the 12 vertices of an icosahedron.
       The vertices are on the unit sphere. The order of the
       vertices are essential for other dependent routines.
     */
{
  double a = 0.850650808352039932;
  double b = 0.525731112119133606;
  vector3 *v, *p;
  int i, j;

  p = v = malloc (12 * sizeof (vector3));

  for (i = 1; i <= 2; i++) {
    a = -a;
    for (j = 1; j <= 2; j++) {
      b = -b;
      v3_initialize (p++, 0.0, a, b);
      v3_initialize (p++, b, 0.0, a);
      v3_initialize (p++, a, b, 0.0);
    }
  }

  return v;
}
