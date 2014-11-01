#ifndef EXTENT3D_H
#define EXTENT3D_H 1

#include <boolean.h>
#include <vector3.h>

void
ext3d_initialize (void);

void
ext3d_push (void);

void
ext3d_pop (boolean transfer);

int
ext3d_depth (void);

void
ext3d_get_center_size (vector3 *center, vector3 *size);

void
ext3d_get_low_high (vector3 *low, vector3 *high);

void
ext3d_set_center_size (vector3 *center, vector3 *size);

void
ext3d_set_low_high (vector3 *low, vector3 *high);

void
ext3d_update (vector3 *p, double radius);

#endif
