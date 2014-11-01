#ifndef VECTOR3_H
#define VECTOR3_H 1

#include <boolean.h>

typedef struct {
  double x, y, z;
} vector3;

void
v3_initialize (vector3 *dest, const double x, const double y, const double z);

void
v3_sum (vector3 *dest, const vector3 *v1, const vector3 *v2);

void
v3_difference (vector3 *dest, const vector3 *v1, const vector3 *v2);

void
v3_scaled (vector3 *dest, const double s, const vector3 *src);

void
v3_add (vector3 *dest, const vector3 *v);

void
v3_subtract (vector3 *dest, const vector3 *v);

void
v3_scale (vector3 *dest, const double s);

void
v3_invert (vector3 *v);

void
v3_reverse (vector3 *v);

void
v3_sum_scaled (vector3 *dest, const vector3 *v1,
	       const double s, const vector3 *v2);

void
v3_add_scaled (vector3 *dest, const double s, const vector3 *v);

double
v3_length (const vector3 *v);

double
v3_angle (const vector3 *v1, const vector3 *v2);

double
v3_angle_points (const vector3 *p1, const vector3 *p2, const vector3 *p3);

double
v3_torsion (const vector3 *p1, const vector3 *v1,
	    const vector3 *p2, const vector3 *v2);

double
v3_torsion_points (const vector3 *p1, const vector3 *p2,
		   const vector3 *p3, const vector3 *p4);

double
v3_dot_product (const vector3 *v1, const vector3 *v2);

void
v3_cross_product (vector3 *vz, const vector3 *vx, const vector3 *vy);

void
v3_triangle_normal (vector3 *n,
		    const vector3 *p1, const vector3 *p2, const vector3 *p3);

double
v3_sqdistance (const vector3 *p1, const vector3 *p2);

double
v3_distance (const vector3 *p1, const vector3 *p2);

boolean
v3_close (const vector3 *v1, const vector3 *v2, const double sqdistance);

void
v3_normalize (vector3 *v);

void
v3_middle (vector3 *dest, const vector3 *p1, const vector3 *p2);

void
v3_between (vector3 *dest,
	    const vector3 *p1, const vector3 *p2, const double fraction);

int
v3_different (const vector3 *v1, const vector3 *v2);

#endif
