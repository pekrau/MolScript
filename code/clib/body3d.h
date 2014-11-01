#ifndef BODY3D_H
#define BODY3D_H 1

#include <vector3.h>

vector3 *
sphere_ico_points (int level);

int
sphere_ico_point_count (int level);

vector3 *
tetrahedron_vertices (void);

vector3 *
octahedron_vertices (void);

vector3 *
cube_vertices (void);

vector3 *
icosahedron_vertices (void);

#endif
