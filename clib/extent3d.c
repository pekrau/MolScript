/*
   3D extent stack routines.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
     8-Mar-1997  started writing
    23-Aug-1997  fixed bugs: size in extent3d_set and ext_count in extent3d_pop
    24-Jun-1998  mod's for hgen, added get/set_low_high, renamed
*/

#include "extent3d.h"

/* public ====================
#include <boolean.h>
#include <vector3.h>
==================== public */

#include <assert.h>
#include <stdlib.h>


/*============================================================*/
typedef struct {
  double xlo, xhi, ylo, yhi, zlo, zhi;
} extent3d;


static extent3d *ext_array = NULL;
static extent3d *curr_ext = NULL;
static int ext_alloc = 8;
static int ext_count = 0;


/*------------------------------------------------------------*/
void
ext3d_initialize (void)
{
  if (ext_array == NULL) {
    ext_array = malloc (ext_alloc * sizeof (extent3d));
    ext_count = 1;
  }

  curr_ext = ext_array + ext_count - 1;

  curr_ext->xlo = 1.0e20;
  curr_ext->xhi = -1.0e20;
  curr_ext->ylo = 1.0e20;
  curr_ext->yhi = -1.0e20;
  curr_ext->zlo = 1.0e20;
  curr_ext->zhi = -1.0e20;

  assert (ext_count >= 1);
  assert (curr_ext);
}


/*------------------------------------------------------------*/
void
ext3d_push (void)
{
  extent3d *ext;
#ifndef NDEBUG
  int old_count = ext_count;
#endif

  assert (curr_ext);

  if (ext_count >= ext_alloc) {
    ext_alloc *= 2;
    ext_array = realloc (ext_array, ext_alloc * sizeof (extent3d));
  }

  ext = ext_array + ext_count - 1;
  ext_count++;
  curr_ext = ext_array + ext_count - 1;
  *curr_ext = *ext;

#ifndef NDEBUG
  assert (ext_count == old_count + 1);
#endif
}


/*------------------------------------------------------------*/
void
ext3d_pop (boolean transfer)
{
  extent3d *ext = curr_ext;
#ifndef NDEBUG
  int old_count = ext_count;
#endif

  assert (curr_ext);
  assert (ext_count > 1);

  ext_count--;
  curr_ext = ext_array + ext_count - 1;

  if (transfer) {
    if (curr_ext->xlo > ext->xlo) curr_ext->xlo = ext->xlo;
    if (curr_ext->xhi < ext->xhi) curr_ext->xhi = ext->xhi;
    if (curr_ext->ylo > ext->ylo) curr_ext->ylo = ext->ylo;
    if (curr_ext->yhi < ext->yhi) curr_ext->yhi = ext->yhi;
    if (curr_ext->zlo > ext->zlo) curr_ext->zlo = ext->zlo;
    if (curr_ext->zhi < ext->zhi) curr_ext->zhi = ext->zhi;
  }

#ifndef NDEBUG
  assert (ext_count == old_count - 1);
#endif
}


/*------------------------------------------------------------*/
int
ext3d_depth (void)
{
  return ext_count;
}


/*------------------------------------------------------------*/
void
ext3d_get_center_size (vector3 *center, vector3 *size)
{
  assert (center);
  assert (size);
  assert (curr_ext);
  assert (ext_count >= 1);

  center->x = 0.5 * (curr_ext->xlo + curr_ext->xhi);
  center->y = 0.5 * (curr_ext->ylo + curr_ext->yhi);
  center->z = 0.5 * (curr_ext->zlo + curr_ext->zhi);
  size->x = curr_ext->xhi - curr_ext->xlo;
  size->y = curr_ext->yhi - curr_ext->ylo;
  size->z = curr_ext->zhi - curr_ext->zlo;
}


/*------------------------------------------------------------*/
void
ext3d_get_low_high (vector3 *low, vector3 *high)
{
  assert (low);
  assert (high);
  assert (curr_ext);
  assert (ext_count >= 1);

  low->x = curr_ext->xlo;
  low->y = curr_ext->ylo;
  low->z = curr_ext->zlo;
  high->x = curr_ext->xhi;
  high->y = curr_ext->yhi;
  high->z = curr_ext->zhi;
}


/*------------------------------------------------------------*/
void
ext3d_set_center_size (vector3 *center, vector3 *size)
{
  assert (center);
  assert (size);
  assert (curr_ext);
  assert (ext_count >= 1);

  curr_ext->xlo = center->x - 0.5 * size->x;
  curr_ext->xhi = center->x + 0.5 * size->x;
  curr_ext->ylo = center->y - 0.5 * size->y;
  curr_ext->yhi = center->y + 0.5 * size->y;
  curr_ext->zlo = center->z - 0.5 * size->z;
  curr_ext->zhi = center->z + 0.5 * size->z;
}


/*------------------------------------------------------------*/
void
ext3d_set_low_high (vector3 *low, vector3 *high)
{
  assert (low);
  assert (high);
  assert (curr_ext);
  assert (ext_count >= 1);

  curr_ext->xlo = low->x;
  curr_ext->xhi = low->y;
  curr_ext->ylo = low->z;
  curr_ext->yhi = high->x;
  curr_ext->zlo = high->y;
  curr_ext->zhi = high->z;
}


/*------------------------------------------------------------*/
void
ext3d_update (vector3 *p, double radius)
{
  register double u;

  assert (p);
  assert (radius >= 0.0);
  assert (radius < 1.0e20);
  assert (curr_ext);
  assert (ext_count >= 1);

  u = p->x - radius;
  if (u < curr_ext->xlo) curr_ext->xlo = u;
  u = p->x + radius;
  if (u > curr_ext->xhi) curr_ext->xhi = u;
  u = p->y - radius;
  if (u < curr_ext->ylo) curr_ext->ylo = u;
  u = p->y + radius;
  if (u > curr_ext->yhi) curr_ext->yhi = u;
  u = p->z - radius;
  if (u < curr_ext->zlo) curr_ext->zlo = u;
  u = p->z + radius;
  if (u > curr_ext->zhi) curr_ext->zhi = u;
}
