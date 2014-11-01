#ifndef HERMITE_CURVE_H
#define HERMITE_CURVE_H 1

#include <vector3.h>

void
hermite_set (vector3 *pos_start, vector3 *pos_finish,
	     vector3 *vec_start, vector3 *vec_finish);

void
hermite_get (vector3 *p, double t);

void
hermite_get_tangent (vector3 *v, double t);

#endif
