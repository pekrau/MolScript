#ifndef VRML_H
#define VRML_H 1

#include <stdio.h>

#include <vector3.h>
#include <colour.h>
#include <indent.h>

void
vrml_initialize (void);

int
vrml_header (void);

void
vrml_comment (char *comment);

void
vrml_worldinfo (char *title, char *info);

void
vrml_navigationinfo (double speed, char *type);

void
vrml_viewpoint (vector3 *pos, vector3 *ori, double rot,
		double fov, char *descr);

void
vrml_fog (int type_exp, double r, colour *c);

void
vrml_s_newline (char *str);

void
vrml_s_quoted (char *str);

void
vrml_i (int i);

void
vrml_f (double d);

void
vrml_g (double d);

void
vrml_f2 (double d);

void
vrml_g3 (double d);

void
vrml_v3 (vector3 *v);

void
vrml_v3_g (vector3 *v);

void
vrml_v3_g3 (vector3 *v);

void
vrml_colour (colour *c);

void
vrml_rgb_colour (colour *c);

void
vrml_tri_indices (int base, int i1, int i2, int i3);

void
vrml_quad_indices (int base, int i1, int i2, int i3, int i4);

void
vrml_begin_node (void);

void
vrml_finish_node (void);

void
vrml_begin_list (void);

void
vrml_finish_list (void);

boolean
vrml_valid_name (char *name);

void
vrml_def (char *def);

void
vrml_proto (char *proto);

void
vrml_node (char *node);

void
vrml_list (char *list);

void
vrml_bbox (vector3 *center, vector3 *size);

void
vrml_material (colour *dc, colour *ec, colour *sc,
	       double ai, double sh, double tr);

void
vrml_directionallight (double in, double ai, colour *c, vector3 *dir);

void
vrml_pointlight (double in, double ai, colour *c,
		 vector3 *loc, double r, vector3 *att);

void
vrml_spotlight (double in, double ai, colour *c,
		vector3 *loc, vector3 *dir, double bw, double coa,
		double r, vector3 *att);

#endif
