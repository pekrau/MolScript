#ifndef QUATERNION_H
#define QUATERNION_H 1

#include <vector3.h>
#include <matrix3.h>

typedef struct {
  vector3 axis;
  double phi;
} quaternion;

void
quat_initialize (quaternion *q, double x, double y, double z, double phi);

void
quat_initialize_v3 (quaternion *q, vector3 *v, double phi);

void
quat_normalize (quaternion *q);

void
quat_add (quaternion *dest, quaternion *incr);

void
quat_to_matrix3 (double m[4][4], quaternion *q);

void
quat_interpolate (quaternion *dest, quaternion *q0, quaternion *q1, double t);

void
quat_trackball (quaternion *dest, double size,
		double p1x, double p1y, double p2x, double p2y);

#endif
