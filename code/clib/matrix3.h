#ifndef MATRIX3_H
#define MATRIX3_H 1

#include <vector3.h>

void
matrix3_initialize (double m[4][4]);

void
matrix3_copy (double dest[4][4], double src[4][4]);

void
matrix3_concatenate (double dest[4][4], double src[4][4]);

void
matrix3_x_rotation (double m[4][4], double radians);

void
matrix3_y_rotation (double m[4][4], double radians);

void
matrix3_z_rotation (double m[4][4], double radians);

void
matrix3_translation (double m[4][4], double x, double y, double z);

void
matrix3_scale (double m[4][4], double sx, double sy, double sz);

void
matrix3_orthographic_projection (double m[4][4],
				 double left, double right,
				 double bottom, double top,
				 double near, double far);

void
matrix3_transform (vector3 *v, double m[4][4]);

void
matrix3_rotate (vector3 *v, double m[4][4]);

void
matrix3_rotate_translate (vector3 *v, double m[4][4]);

#endif
