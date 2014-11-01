/* graphics.h

   MolScript v2.1.2

   Graphics: construct the geometries and call the output procedures.

   Copyright (C) 1997-1998 Per Kraulis
     6-Dec-1996  first attempts
*/

#ifndef GRAPHICS_H
#define GRAPHICS_H 1

#include "coord.h"
#include "state.h"

enum object_codes {OBJ_POINTS, OBJ_POINTS_COLOURS,
		   OBJ_LINES, OBJ_LINES_COLOURS,
		   OBJ_TRIANGLES, OBJ_TRIANGLES_COLOURS, OBJ_TRIANGLES_NORMALS,
		   OBJ_TRIANGLES_NORMALS_COLOURS,
		   OBJ_STRIP, OBJ_STRIP_COLOURS, OBJ_STRIP_NORMALS,
		   OBJ_STRIP_NORMALS_COLOURS };

#define LINEWIDTH_FACTOR 0.04
#define LINEWIDTH_MINIMUM 0.005

extern boolean frame;
extern double area [4];
extern colour background_colour;
extern double window;
extern double slab;
extern boolean headlight;
extern boolean shadows;
extern double fog;
extern double dynamics_time;

extern double aspect_ratio;
extern double aspect_window_x, aspect_window_y;

extern void (*output_first_plot) (void);
extern void (*output_start_plot) (void);
extern void (*output_finish_plot) (void);
extern void (*output_finish_output) (void);

extern void (*set_area) (void);
extern void (*set_background) (void);
extern void (*anchor_start) (char *str);
extern void (*anchor_description) (char *str);
extern void (*anchor_parameter) (char *str);
extern void (*anchor_start_geometry) (void);
extern void (*anchor_finish) (void);
extern void (*lod_start) (void);
extern void (*lod_finish) (void);
extern void (*lod_start_group) (void);
extern void (*lod_finish_group) (void);
extern void (*viewpoint_start) (char *str);
extern void (*viewpoint_output) (void);
extern void (*output_directionallight) (void);
extern void (*output_pointlight) (void);
extern void (*output_spotlight) (void);
extern void (*output_comment) (char *str);

extern void (*output_coil) (void);
extern void (*output_cylinder) (vector3 *v1, vector3 *v2);
extern void (*output_helix) (void);
extern void (*output_label) (vector3 *p, char *label, colour *c);
extern void (*output_line) (boolean polylines);
extern void (*output_sphere) (at3d *at, double radius);
extern void (*output_stick) (vector3 *v1, vector3 *v2,
			     double r1, double r2, colour *c);
extern void (*output_strand) (void);

extern void (*output_start_object) (void);
extern void (*output_object) (int code, vector3 *triplets, int count);
extern void (*output_finish_object) (void);

extern void (*output_pickable) (at3d *atom);

void graphics_plot_init (void);
void set_area_values (double xlo, double ylo, double xhi, double yhi);
void set_window (void);
void set_slab (void);
void set_fog (void);

void set_extent (void);
int outside_extent_radius (vector3 *v, double radius);
int outside_extent_2v (vector3 *v1, vector3 *v2);

double depthcue (double depth, state *st);

void ball_and_stick (int single_selection);
void bonds (int single_selection);
void coil (int is_peptide_chain, int smoothing);
void cpk (void);
void cylinder (void);
void helix (void);
void label_atoms (char *label);
void label_position (char *label);
void line_start (void);
void line_next (void);
void object (char *filename);
void strand (void);
void trace (void);

#endif
