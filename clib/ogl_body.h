#ifndef OGL_BODY_H
#define OGL_BODY_H 1

#include <vector3.h>

void
ogl_sphere_vertices_ico (vector3 *pos, double radius, int level);

void
ogl_sphere_faces_ico_recursive (vector3 *pos, double radius, int depth);

void
ogl_sphere_faces_globe (vector3 *pos, double radius, int segments);

void
ogl_tetrahedron_vertices (vector3 *pos, double radius);

void
ogl_tetrahedron_edges (vector3 *pos, double radius);

void
ogl_tetrahedron_faces (vector3 *pos, double radius);

void
ogl_octahedron_vertices (vector3 *pos, double radius);

void
ogl_octahedron_edges (vector3 *pos, double radius);

void
ogl_octahedron_faces (vector3 *pos, double radius);

void
ogl_cube_vertices (vector3 *pos, double radius);

void
ogl_cube_edges (vector3 *pos, double radius);

void
ogl_cube_faces (vector3 *pos, double radius);

void
ogl_icosahedron_vertices (vector3 *pos, double radius);

void
ogl_icosahedron_edges (vector3 *pos, double radius);

void
ogl_icosahedron_faces (vector3 *pos, double radius);

void
ogl_cylinder_faces (vector3 *pos1, vector3 *pos2,
		    double radius, int segments, boolean capped);

#endif
