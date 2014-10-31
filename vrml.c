/* vrml.c

   MolScript v2.1.2

   VRML V2.0

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
    31-Mar-1997  fairly finished
    18-Aug-1997  split out and generalized segment handling
    15-Oct-1997  USE/DEF for helix and strand different surfaces
    25-Jan-1998  prepare some proto's for dynamics
    20-Aug-1998  fixed level-of-detail bug
    28-Aug-1998  split out indent functions
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "clib/angle.h"
#include "clib/extent3d.h"
#include "clib/str_utils.h"
#include "clib/dynstring.h"
#include "clib/vrml.h"

#include "vrml.h"
#include "global.h"
#include "graphics.h"
#include "segment.h"
#include "state.h"


/*============================================================*/
typedef struct s_viewpoint_node viewpoint_node;

struct s_viewpoint_node {
  vector3 from;
  vector3 ori;
  double rot;
  double fov;
  char *descr;
  viewpoint_node *next;
};

static viewpoint_node *viewpoints = NULL;
static char *viewpoint_str = NULL;

static int line_count;
static int point_count;
static int polygon_count;
static int colour_count;	/* segment colours */
static int cylinder_count;
static int sphere_count;
static int label_count;
static int def_count;

#define STICK_SEGMENTS 8

#define MAX_COLOUR_CACHE 16
#define COLOUR_MAXDIFF 0.005
static colour colour_cache[MAX_COLOUR_CACHE];
static colour colour_cache_latest;
static int colour_cache_count;
static int colour_cache_index_count;

static int anchor_parameter_not_started = TRUE;

static double *lod_ranges = NULL;
static int lod_alloc;
static int lod_count;


/*------------------------------------------------------------*/
static void
output_rot (vector3 *axis, double angle)
{
  assert (axis);

  indent_string ("r ");
  vrml_v3_g (axis);
  vrml_g (angle);
}


/*------------------------------------------------------------*/
static void
output_scale (double x, double y, double z)
{
  assert (x > 0.0);
  assert (y > 0.0);
  assert (z > 0.0);

  indent_string ("s ");
  vrml_g (x);
  vrml_g (y);
  vrml_g (z);
}


/*------------------------------------------------------------*/
static void
output_dc (colour *dc)
{
  assert (dc);

  if (colour_unequal (dc, &white_colour)) {
    indent_string ("dc ");
    vrml_colour (dc);
  }
}


/*------------------------------------------------------------*/
static void
output_ec (colour *ec)
{
  assert (ec);

  if (colour_unequal (ec, &black_colour)) {
    indent_string ("ec ");
    vrml_colour (ec);
  }
}


/*------------------------------------------------------------*/
static void
output_sc (colour *sc)
{
  assert (sc);

  if (colour_unequal (sc, &grey02_colour)) {
    indent_string ("sc ");
    vrml_colour (sc);
  }
}


/*------------------------------------------------------------*/
static void
output_sh (double sh)
{
  assert (sh >= 0.0);
  assert (sh <= 1.0);

  if (sh != 0.0) {
    indent_string ("sh");
    vrml_g3 (sh);
  }
}


/*------------------------------------------------------------*/
static void
output_tr (double tr)
{
  assert (tr >= 0.0);
  assert (tr <= 1.0);

  if (tr != 0.0) {
    indent_string ("tr");
    vrml_g3 (tr);
  }
}


/*------------------------------------------------------------*/
static void
output_appearance (int is_line, colour *c)
{
  assert (c);

  indent_newline_conditional();
  vrml_node ("appearance Appearance");
  indent_string ("material");

  if (current_state->colourparts) {
    if (is_line) {
      vrml_material (NULL, NULL, &(current_state->specularcolour),
		     0.2, current_state->shininess,
		     current_state->transparency);
    } else {
      vrml_material (NULL, &(current_state->emissivecolour),
		     &(current_state->specularcolour),
		     0.2, current_state->shininess,
		     current_state->transparency);
    }

  } else {
    if (is_line) {
      vrml_material (NULL, c, &(current_state->specularcolour),
		     0.2, current_state->shininess,
		     current_state->transparency);
    } else {
      vrml_material (c, &(current_state->emissivecolour),
		     &(current_state->specularcolour),
		     0.2, current_state->shininess,
		     current_state->transparency);
    }
  }

  vrml_finish_node();
}


/*------------------------------------------------------------*/
static void
colour_cache_init (void)
{
  colour_cache_count = 0;
}


/*------------------------------------------------------------*/
static void
colour_cache_add (colour *c)
{
  int slot;

  assert (c);

  colour_to_rgb (c);

  for (slot = colour_cache_count - 1; slot >= 0; slot--) {
    if (colour_approx_equal (c, &(colour_cache[slot]), COLOUR_MAXDIFF)) return;
  }

  if (colour_cache_count < MAX_COLOUR_CACHE) {
    colour_cache_index_count = colour_cache_count;
    colour_cache[colour_cache_count++] = *c;
  } else if (colour_approx_equal (c, &colour_cache_latest, COLOUR_MAXDIFF)) {
    return;
  }

  vrml_rgb_colour (c);
  colour_count++;
  colour_cache_latest = *c;
}


/*------------------------------------------------------------*/
static void
colour_cache_index_init (void)
{
  colour_cache_latest.spec = -1;
}


/*------------------------------------------------------------*/
static int
colour_cache_index (colour *c)
{
  int slot;

  assert (c);

  for (slot = colour_cache_count - 1; slot >= 0; slot--) {
    if (colour_approx_equal (c, &(colour_cache[slot]), COLOUR_MAXDIFF)) {
      return slot;
    }
  }

  if (colour_approx_equal (c, &colour_cache_latest, COLOUR_MAXDIFF)) {
    return colour_cache_index_count;
  } else {
    colour_cache_latest = *c;
    return ++colour_cache_index_count;
  }
}


/*------------------------------------------------------------*/
void
vrml_set (void)
{
  output_first_plot = vrml_first_plot;
  output_start_plot = vrml_start_plot;
  output_finish_plot = vrml_finish_plot;
  output_finish_output = vrml_finish_output;

  set_area = vrml_set_area;
  set_background = vrml_set_background;
  anchor_start = vrml_anchor_start;
  anchor_description = vrml_anchor_description;
  anchor_parameter = vrml_anchor_parameter;
  anchor_start_geometry = vrml_anchor_start_geometry;
  anchor_finish = vrml_anchor_finish;
  lod_start = vrml_lod_start;
  lod_finish = vrml_lod_finish;
  lod_start_group = vrml_lod_start_group;
  lod_finish_group = vrml_lod_finish_group;
  viewpoint_start = vrml_viewpoint_start;
  viewpoint_output = vrml_viewpoint_output;
  output_directionallight = vrml_ms_directionallight;
  output_pointlight = vrml_ms_pointlight;
  output_spotlight = vrml_ms_spotlight;
  output_comment = vrml_comment;

  output_coil = vrml_coil;
  output_cylinder = vrml_cylinder;
  output_helix = vrml_helix;
  output_label = vrml_label;
  output_line = vrml_line;
  output_sphere = vrml_sphere;
  output_stick = vrml_stick;
  output_strand = vrml_strand;

  output_start_object = do_nothing;
  output_object = vrml_object;
  output_finish_object = do_nothing;

  output_pickable = NULL;

  constant_colours_to_rgb();

  output_mode = VRML_MODE;
}


/*------------------------------------------------------------*/
void
vrml_first_plot (void)
{
  int slot;
  double angle;

  if (!first_plot)
    yyerror ("only one plot per input file allowed for VRML 2.0");

  set_outfile ("w");
  indent_initialize (outfile);
  indent_flag = pretty_format;

  if (vrml_header() < 0) yyerror ("could not write to VRML 2.0 file");

  vrml_node ("WorldInfo");
  if (title) {
    indent_string ("title ");
    vrml_s_quoted (title);
    indent_newline();
  }
  vrml_list ("info");
  INDENT_FPRINTF (indent_file,
		  "\"Program: %s, %s\"", program_str, copyright_str);
  if (user_str[0] != '\0') {
    indent_newline();
    INDENT_FPRINTF (indent_file, "\"Author: %s\"", user_str);
  }
  vrml_finish_list();
  vrml_finish_node();

  indent_newline();
  vrml_comment ("MolScript: begin proto definitions");
  vrml_proto ("Ball");
  vrml_s_newline ("exposedField SFVec3f p 0 0 0");
  vrml_s_newline ("field SFFloat rad 1.5");
  vrml_s_newline ("field SFColor dc 1 1 1");
  vrml_s_newline ("field SFColor ec 0 0 0");
  vrml_s_newline ("field SFColor sc 0.2 0.2 0.2");
  vrml_s_newline ("field SFFloat sh 0");
  indent_string ("field SFFloat tr 0");
  vrml_finish_list();
  indent_newline();
  vrml_begin_node();
  vrml_node ("Transform");
  vrml_s_newline ("translation IS p");
  vrml_list ("children");
  vrml_node ("Shape");
  vrml_node ("appearance Appearance");
  vrml_node ("material Material");
  vrml_s_newline ("diffuseColor IS dc");
  vrml_s_newline ("emissiveColor IS ec");
  vrml_s_newline ("specularColor IS sc");
  vrml_s_newline ("shininess IS sh");
  indent_string ("transparency IS tr");
  vrml_finish_node();
  vrml_finish_node();
  indent_newline();
  vrml_node ("geometry Sphere");
  indent_string ("radius IS rad");
  vrml_finish_node();
  vrml_finish_node();
  vrml_finish_list();
  vrml_finish_node();
  vrml_finish_node();

  vrml_proto ("Cyl");
  vrml_s_newline ("exposedField SFVec3f p 0 0 0");
  vrml_s_newline ("exposedField SFRotation r 0 0 1 0");
  vrml_s_newline ("exposedField SFVec3f s 1 1 1");
  vrml_s_newline ("field SFColor dc 1 1 1");
  vrml_s_newline ("field SFColor ec 0 0 0");
  vrml_s_newline ("field SFColor sc 0.2 0.2 0.2");
  vrml_s_newline ("field SFFloat sh 0");
  indent_string ("field SFFloat tr 0");
  vrml_finish_list();
  indent_newline();
  vrml_begin_node();
  vrml_node ("Transform");
  vrml_s_newline ("translation IS p");
  vrml_s_newline ("rotation IS r");
  vrml_s_newline ("scale IS s");
  vrml_list ("children");
  vrml_node ("Shape");
  vrml_node ("appearance Appearance");
  vrml_node ("material Material");
  vrml_s_newline ("diffuseColor IS dc");
  vrml_s_newline ("emissiveColor IS ec");
  vrml_s_newline ("specularColor IS sc");
  vrml_s_newline ("shininess IS sh");
  indent_string ("transparency IS tr");
  vrml_finish_node();
  vrml_finish_node();
  indent_newline();
  indent_string ("geometry Cylinder { }");
  vrml_finish_node();
  vrml_finish_list();
  vrml_finish_node();
  vrml_finish_node();

  vrml_proto ("Stick");
  vrml_s_newline ("exposedField SFVec3f p 0 0 0");
  vrml_s_newline ("exposedField SFRotation r 0 0 1 0");
  vrml_s_newline ("exposedField SFVec3f s 1 1 1");
  vrml_s_newline ("field SFColor dc 1 1 1");
  vrml_s_newline ("field SFColor ec 0 0 0");
  vrml_s_newline ("field SFColor sc 0.2 0.2 0.2");
  vrml_s_newline ("field SFFloat sh 0");
  indent_string ("field SFFloat tr 0");
  vrml_finish_list();
  indent_newline();
  vrml_begin_node();
  vrml_node ("Transform");
  vrml_s_newline ("translation IS p");
  vrml_s_newline ("rotation IS r");
  vrml_s_newline ("scale IS s");
  vrml_list ("children");
  vrml_node ("Shape");
  vrml_node ("appearance Appearance");
  vrml_node ("material Material");
  vrml_s_newline ("diffuseColor IS dc");
  vrml_s_newline ("emissiveColor IS ec");
  vrml_s_newline ("specularColor IS sc");
  vrml_s_newline ("shininess IS sh");
  indent_string ("transparency IS tr");
  vrml_finish_node();
  vrml_finish_node();
  indent_newline();
  vrml_node ("geometry Extrusion");
  vrml_s_newline ("spine [0 -1 0, 0 1 0]");
  vrml_list ("crossSection");
  vrml_g (1.0);
  vrml_g (0.0);
  for (slot = STICK_SEGMENTS - 1; slot > 0; slot--) {
    angle = 2.0 * ANGLE_PI * (double) slot / (double) STICK_SEGMENTS;
    vrml_g (cos (angle));
    vrml_g (sin (angle));
  }
  vrml_g (1.0);
  vrml_g (0.0);
  vrml_finish_list();
  indent_newline();
  vrml_s_newline ("beginCap FALSE");
  vrml_s_newline ("endCap FALSE");
  indent_string ("creaseAngle 1.7");
  vrml_finish_node();
  vrml_finish_node();
  vrml_finish_list();
  vrml_finish_node();
  vrml_finish_node();

  vrml_proto ("Label");
  vrml_s_newline ("field SFVec3f p 0 0 0");
  vrml_s_newline ("field SFFloat sz 1");
  vrml_s_newline ("field MFString c []");
  vrml_s_newline ("field SFVec3f o 0 0 0");
  vrml_s_newline ("field SFColor dc 1 1 1");
  vrml_s_newline ("field SFColor ec 0 0 0");
  vrml_s_newline ("field SFColor sc 0.2 0.2 0.2");
  vrml_s_newline ("field SFFloat sh 0");
  indent_string ("field SFFloat tr 0");
  vrml_finish_list();
  indent_newline();
  vrml_begin_node();
  vrml_node ("Transform");
  vrml_s_newline ("translation IS p");
  vrml_list ("children");
  vrml_node ("Billboard");
  vrml_s_newline ("axisOfRotation 0 0 0");
  vrml_list ("children");
  vrml_node ("Transform");
  vrml_s_newline ("translation IS o");
  vrml_list ("children");
  vrml_node ("Shape");
  vrml_node ("appearance Appearance");
  vrml_node ("material Material");
  vrml_s_newline ("diffuseColor IS dc");
  vrml_s_newline ("emissiveColor IS ec");
  vrml_s_newline ("specularColor IS sc");
  vrml_s_newline ("shininess IS sh");
  indent_string ("transparency IS tr");
  vrml_finish_node();
  vrml_finish_node();
  indent_newline();
  vrml_node ("geometry Text");
  vrml_s_newline ("string IS c");
  vrml_node ("fontStyle FontStyle");
  indent_string ("size IS sz");
  vrml_finish_node();
  vrml_finish_node();
  vrml_finish_node();
  vrml_finish_list();
  vrml_finish_node();
  vrml_finish_list();
  vrml_finish_node();
  vrml_finish_list();
  vrml_finish_node();
  vrml_finish_node();
  indent_newline();
  vrml_comment ("MolScript: end proto definitions");
}


/*------------------------------------------------------------*/
void
vrml_finish_output (void)
{
  indent_newline();
  vrml_comment ("MolScript: end of output");
}


/*------------------------------------------------------------*/
void
vrml_start_plot (void)
{
  set_area_values (-1.0, -1.0, 1.0, 1.0);

  line_count = 0;
  point_count = 0;
  polygon_count = 0;
  colour_count = 0;
  cylinder_count = 0;
  sphere_count = 0;
  label_count = 0;
  def_count = 0;

  colour_copy_to_rgb (&background_colour, &black_colour);

  indent_newline();
  vrml_comment ("MolScript: collision detection switched off for all objects");
  vrml_node ("Collision");
  vrml_s_newline ("collide FALSE");
  if (indent_level == 0) {
    vrml_list ("children");
  } else {
    indent_string ("children [");
    indent_increment_level (TRUE);
  }
}


/*------------------------------------------------------------*/
void
vrml_finish_plot (void)
{
  vector3 center, size, pos;
  viewpoint_node *vp, *vp2;

  vrml_finish_list();

  set_extent();

  ext3d_get_center_size (&center, &size);
  size.x += 2.0;		/* add a safety margin to the bounding box */
  size.y += 2.0;
  size.z += 2.0;
  vrml_bbox (&center, &size);

  vrml_finish_node();

  indent_newline();
  vrml_node ("NavigationInfo");
  indent_string ("speed ");
  vrml_g (4.0);
  indent_newline();
  vrml_list ("type");
  vrml_s_quoted ("EXAMINE");
  vrml_s_quoted ("FLY");
  vrml_finish_list();
  if (!headlight) {
    indent_newline();
    indent_string ("headlight");
    indent_string ("FALSE");
  }
  vrml_finish_node();

  if (colour_unequal (&background_colour, &black_colour)) {
    indent_newline();
    vrml_node ("Background");
    vrml_list ("skyColor");
    vrml_colour (&background_colour);
    vrml_finish_list();
    vrml_finish_node();
  }

  indent_newline();
  pos = center;
  if (size.x > size.y) {
    pos.z += 1.2 * size.x;
  } else {
    pos.z += 1.2 * size.y;
  }
  vrml_viewpoint (&pos, NULL, 0.0, 0.0, "overall default");

  if (fog != 0.0) vrml_fog (FALSE, fog, &background_colour);

  for (vp = viewpoints; vp; vp = vp2) {
    if (vp->rot > 0.0001) {
      vrml_viewpoint (&(vp->from), &(vp->ori), vp->rot, vp->fov, vp->descr);
    } else {
      vrml_viewpoint (&(vp->from), NULL, 0.0, vp->fov, vp->descr);
    }
    vp2 = vp->next;
    free (vp->descr);
    free (vp);
  }
  viewpoints = NULL;

  if (message_mode) {
    fprintf (stderr, "%i line coordinates, %i points, %i polygons, %i segment colours,\n",
	     line_count, point_count, polygon_count, colour_count);
    fprintf (stderr, "%i cylinders, %i spheres and %i labels.\n",
	     cylinder_count, sphere_count, label_count);
  }
}


/*------------------------------------------------------------*/
void
vrml_anchor_start (char *str)
{
  assert (str);
  assert (*str);

  indent_newline();
  vrml_node ("Anchor");
  indent_string ("url ");
  vrml_s_quoted (str);
}


/*------------------------------------------------------------*/
void
vrml_anchor_description (char *str)
{
  assert (str);
  assert (*str);

  indent_newline();
  indent_string ("description ");
  vrml_s_quoted (str);
}


/*------------------------------------------------------------*/
void
vrml_anchor_parameter (char *str)
{
  assert (str);
  assert (*str);

  indent_newline();
  if (anchor_parameter_not_started) {
    vrml_list ("parameter");
    anchor_parameter_not_started = FALSE;
  }
  vrml_s_quoted (str);
}


/*------------------------------------------------------------*/
void
vrml_anchor_start_geometry (void)
{
  if (!anchor_parameter_not_started) {
    vrml_finish_list();
    anchor_parameter_not_started = TRUE;
  }
  indent_newline();
  if (indent_level <= 0) {
    vrml_list ("children");
  } else {
    indent_string ("children [");
    indent_increment_level (TRUE);
  }

  ext3d_push();
  ext3d_initialize();
}


/*------------------------------------------------------------*/
void
vrml_anchor_finish (void)
{
  vector3 center, size;

  vrml_finish_list();

  ext3d_get_center_size (&center, &size);
  ext3d_pop (TRUE);
  if (size.x < 0.0) yyerror ("anchor has no clickable objects");
  size.x += 1.0;		/* add a safety margin to the bounding box */
  size.y += 1.0;
  size.z += 1.0;
  vrml_bbox (&center, &size);

  vrml_finish_node();
}


/*------------------------------------------------------------*/
void
vrml_lod_start (void)
{
  indent_newline();
  vrml_node ("LOD");
  vrml_list ("level");

  ext3d_push();
  ext3d_initialize();

  if (lod_alloc == 0) {
    lod_alloc = 8;
    lod_ranges = malloc (lod_alloc * sizeof (double));
  }
  lod_count = 0;
}


/*------------------------------------------------------------*/
void
vrml_lod_finish (void)
{
  vector3 center, size;
  int slot;

  assert (lod_ranges);

  vrml_finish_list();

  ext3d_get_center_size (&center, &size);
  ext3d_pop (TRUE);

  indent_newline();
  indent_string ("center");
  vrml_f2 (center.x);
  vrml_f2 (center.y);
  vrml_f2 (center.z);

  indent_newline();
  vrml_list ("range");
  for (slot = 0; slot < lod_count; slot++) vrml_f2 (lod_ranges[slot]);
  vrml_finish_list();

  vrml_finish_node();
}


/*------------------------------------------------------------*/
void
vrml_lod_start_group (void)
{
  assert (lod_ranges);
  assert ((dstack_size == 1) || (dstack_size == 0));

  if (dstack_size == 1) {
    if (lod_count + 1 >= lod_alloc) {
      lod_alloc *= 2;
      lod_ranges = realloc (lod_ranges, lod_alloc * sizeof (double));
    }

    lod_ranges[lod_count++] = dstack[0];
    clear_dstack();

    if (lod_count > 1 &&
	lod_ranges[lod_count - 1] <= lod_ranges[lod_count - 2])
      yyerror ("level-of-detail range not in ascending order");
  }

  indent_newline();
  vrml_node ("Group");
  vrml_list ("children");

  assert (dstack_size == 0);
}


/*------------------------------------------------------------*/
void
vrml_lod_finish_group (void)
{
  assert (lod_ranges);

  vrml_finish_list();
  vrml_finish_node();
}


/*------------------------------------------------------------*/
void
vrml_viewpoint_start (char *str)
{
  assert (str);
  assert (*str);

  viewpoint_str = str_clone (str);
}


/*------------------------------------------------------------*/
void
vrml_viewpoint_output (void)
{
  viewpoint_node *vp;

  vector3 towards, from, dir;
  vector3 zdir = {0.0, 0.0, -1.0};
  double angle;

  assert (viewpoint_str);
  assert ((dstack_size == 4) ||
	  (dstack_size == 6) ||
	  (dstack_size == 7));

  vp = malloc (sizeof (viewpoint_node));
  vp->next = NULL;

  if (dstack_size == 4) {	/* towards origin */
    towards.x = dstack[0];
    towards.y = dstack[1];
    towards.z = dstack[2];
    dir = towards;
    v3_normalize (&dir);
    v3_sum_scaled (&from, &towards, dstack[3], &dir);

  } else {			/* to, from specified points */
    from.x = dstack[0];
    from.y = dstack[1];
    from.z = dstack[2];
    towards.x = dstack[3];
    towards.y = dstack[4];
    towards.z = dstack[5];
    if (dstack_size == 7) {	/* additional distance */
      v3_difference (&dir, &from, &towards);
      v3_normalize (&dir);
      v3_add_scaled (&from, dstack[6], &dir);
    }
  }
  clear_dstack();

  v3_difference (&dir, &towards, &from);
  angle = v3_angle (&dir, &zdir);

  vp->from = from;
  vp->rot = angle;
  vp->fov = 0.0;
  if (angle > 0.0001) {
    v3_cross_product (&(vp->ori), &zdir, &dir);
    v3_normalize (&(vp->ori));
  }
  vp->descr = viewpoint_str;
  viewpoint_str = NULL;		/* do not delete the string */

  if (viewpoints == NULL) {
    viewpoints = vp;
  } else {
    viewpoint_node *vp2;
    for (vp2 = viewpoints; vp2->next; vp2 = vp2->next) ;
    vp2->next = vp;
  }

  assert (dstack_size == 0);
}


/*------------------------------------------------------------*/
void
vrml_ms_directionallight (void)
{
  vector3 dir;

  assert ((dstack_size == 3) || (dstack_size == 6));

  if (dstack_size == 3) {
    dir.x = dstack[0];
    dir.y = dstack[1];
    dir.z = dstack[2];
  } else {
    dir.x = dstack[3] - dstack[0];
    dir.y = dstack[4] - dstack[1];
    dir.z = dstack[5] - dstack[2];
  }
  v3_normalize (&dir);
  clear_dstack();

  indent_newline();
  vrml_directionallight (current_state->lightintensity,
			 current_state->lightambientintensity,
			 &(current_state->lightcolour),
			 &dir);
}


/*------------------------------------------------------------*/
void
vrml_ms_pointlight (void)
{
  vector3 pos;

  assert (dstack_size == 3);

  pos.x = dstack[0];
  pos.y = dstack[1];
  pos.z = dstack[2];
  clear_dstack();

  indent_newline();
  vrml_pointlight (current_state->lightintensity,
		   current_state->lightambientintensity,
		   &(current_state->lightcolour),
		   &pos,
		   current_state->lightradius,
		   &(current_state->lightattenuation));

  assert (dstack_size == 0);
}


/*------------------------------------------------------------*/
void
vrml_ms_spotlight (void)
{
  vector3 pos, dir;
  double angle;

  assert ((dstack_size == 7) || (dstack_size == 10));

  pos.x = dstack[0];
  pos.y = dstack[1];
  pos.z = dstack[2];

  if (dstack_size == 7) {
    dir.x = dstack[3];
    dir.y = dstack[4];
    dir.z = dstack[5];
    angle = dstack[6];
  } else {
    dir.x = dstack[6] - dstack[3];
    dir.y = dstack[7] - dstack[4];
    dir.z = dstack[8] - dstack[5];
    angle = dstack[9];
  }
  v3_normalize (&dir);
  clear_dstack();

  indent_newline();
  vrml_spotlight (current_state->lightintensity,
		  current_state->lightambientintensity,
		  &(current_state->lightcolour),
		  &pos, &dir,
		  to_radians (angle), to_radians (angle) / 2.0,
		  current_state->lightradius,
		  &(current_state->lightattenuation));

  assert (dstack_size == 0);
}


/*------------------------------------------------------------*/
void
vrml_set_area (void)
{
  if (message_mode) fprintf (stderr, "ignoring 'area' for VRML 2.0 output\n");
  clear_dstack();
}


/*------------------------------------------------------------*/
void
vrml_set_background (void)
{
  colour_copy_to_rgb (&background_colour, &given_colour);
}


/*------------------------------------------------------------*/
void
vrml_coil (void)
{
  int slot, base;
  coil_segment *cs;

  indent_newline();
  vrml_comment ("MolScript: coil or turn");
  vrml_node ("Shape");
  output_appearance (FALSE, &(current_state->planecolour));
  indent_newline();
  vrml_node ("geometry IndexedFaceSet");
  vrml_s_newline ("creaseAngle 2.5");

  vrml_node ("coord Coordinate");
  vrml_list ("point");
  for (slot = 0; slot < coil_segment_count; slot++) {
    cs = coil_segments + slot;
    vrml_v3 (&(cs->p1));
    vrml_v3 (&(cs->p2));
    vrml_v3 (&(cs->p3));
    vrml_v3 (&(cs->p4));
  }
  vrml_finish_list();
  vrml_finish_node();

  indent_newline();
  vrml_list ("coordIndex");
  for (slot = 0; slot < coil_segment_count - 1; slot++) {
    base = 4 * slot;
    vrml_tri_indices (base, 0, 1, 5);
    vrml_tri_indices (base, 0, 5, 4);
    vrml_tri_indices (base, 3, 0, 4);
    vrml_tri_indices (base, 3, 4, 7);
    vrml_tri_indices (base, 2, 3, 7);
    vrml_tri_indices (base, 2, 7, 6);
    vrml_tri_indices (base, 1, 2, 5);
    vrml_tri_indices (base, 2, 6, 5);
  }
  polygon_count += 8 * coil_segment_count;
  base = 0;
  vrml_quad_indices (base, 3, 2, 1, 0);
  base = 4 * (coil_segment_count - 1);
  vrml_quad_indices (base, 3, 2, 1, 0);
  polygon_count += 2;
  vrml_finish_list();

  if (current_state->colourparts) {
    colour rgb;
    int index;

    indent_newline();
    vrml_s_newline ("colorPerVertex FALSE");
    vrml_node ("color Color");
    vrml_list ("color");

    colour_to_rgb (&(coil_segments[0].c));
    rgb = coil_segments[0].c;
    vrml_rgb_colour (&rgb);

    for (slot = 1; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;

      colour_to_rgb (&(cs->c));
      if (colour_unequal (&rgb, &(cs->c))) {
	rgb = cs->c;
	vrml_rgb_colour (&rgb);
	colour_count++;
      }
    }

    vrml_finish_list();
    vrml_finish_node();

    indent_newline();
    vrml_list ("colorIndex");

    rgb = coil_segments[0].c;
    index = 0;
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;

      if (colour_unequal (&rgb, &(cs->c))) {
	rgb = cs->c;
	index++;
      }
      vrml_i (index);
      vrml_i (index);
      vrml_i (index);
      vrml_i (index);
      vrml_i (index);
      vrml_i (index);
      vrml_i (index);
      vrml_i (index);
    }
    vrml_i (0);
    vrml_i (index);

    vrml_finish_list();
  }

  vrml_finish_node();
  vrml_finish_node();
}


/*------------------------------------------------------------*/
void
vrml_cylinder (vector3 *v1, vector3 *v2)
{
  vector3 dir, rot, middle;
  double angle;

  assert (v1);
  assert (v2);
  assert (v3_distance (v1, v2) > 0.0);

  v3_difference (&dir, v1, v2);
  angle = v3_angle (&dir, &yaxis);
  if (angle > 0.0) {
    v3_cross_product (&rot, &yaxis, &dir);
    v3_normalize (&rot);
  } else {
    rot = yaxis;
  }
  v3_middle (&middle, v1, v2);

  indent_newline();
  vrml_node ("Cyl");
  indent_string ("p ");
  vrml_v3 (&middle);
  output_rot (&rot, angle);
  output_scale (current_state->cylinderradius,
		v3_length (&dir) / 2.0,
		current_state->cylinderradius);
  output_dc (&(current_state->planecolour));
  output_ec (&(current_state->emissivecolour));
  output_sc (&(current_state->specularcolour));
  output_sh (current_state->shininess);
  output_tr (current_state->transparency);
  vrml_finish_node();

  cylinder_count++;
}


/*------------------------------------------------------------*/
void
vrml_helix (void)
{
  int slot, base;
  helix_segment *hs;
  dynstring *def_name = NULL;

  indent_newline();
  if (current_state->colourparts) {
    vrml_comment ("MolScript: helix");
  } else {
    vrml_comment ("MolScript: helix outer surface");
  }
  vrml_node ("Shape");
  output_appearance (FALSE, &(current_state->planecolour));
  indent_newline();
  vrml_node ("geometry IndexedFaceSet");
  vrml_s_newline ("creaseAngle 1.6");
  if (current_state->colourparts) vrml_s_newline ("solid FALSE");

  indent_string ("coord");
  if (!current_state->colourparts) {
    def_name = ds_create ("_");
    ds_cat_int (def_name, def_count++);
    vrml_def (def_name->string);
  }
  vrml_node ("Coordinate");
  vrml_list ("point");
  for (slot = 0; slot < helix_segment_count; slot++) {
    hs = helix_segments + slot;
    vrml_v3 (&(hs->p1));
    vrml_v3 (&(hs->p2));
  }
  vrml_finish_list();
  vrml_finish_node();

  indent_newline();
  vrml_list ("coordIndex");
  for (slot = 0; slot < helix_segment_count - 1; slot++) {
    base = 2 * slot;
    vrml_tri_indices (base, 0, 1, 2);
    vrml_tri_indices (base, 1, 3, 2);
  }
  vrml_finish_list();
  polygon_count += 2 * helix_segment_count;

  if (current_state->colourparts) {
    colour rgb;
    int index;

    indent_newline();
    vrml_s_newline ("colorPerVertex FALSE");
    vrml_node ("color Color");
    vrml_list ("color");

    colour_to_rgb (&(helix_segments[0].c));
    rgb = helix_segments[0].c;
    vrml_rgb_colour (&rgb);

    for (slot = 1; slot < helix_segment_count; slot++) {
      hs = helix_segments + slot;

      colour_to_rgb (&(hs->c));
      if (colour_unequal (&rgb, &(hs->c))) {
	rgb = hs->c;
	vrml_rgb_colour (&rgb);
	colour_count++;
      }
    }

    vrml_finish_list();
    vrml_finish_node();

    indent_newline();
    vrml_list ("colorIndex");

    rgb = helix_segments[0].c;
    index = 0;
    for (slot = 0; slot < helix_segment_count; slot++) {
      hs = helix_segments + slot;

      if (colour_unequal (&rgb, &(hs->c))) {
	rgb = hs->c;
	index++;
      }
      vrml_i (index);
      vrml_i (index);
    }

    vrml_finish_list();

  } else {			/* not colourparts */

    vrml_finish_node();		/* finish Shape node for outer surface */
    vrml_finish_node();

    indent_newline();
    vrml_comment ("MolScript: helix inner surface");
    vrml_node ("Shape");
    output_appearance (FALSE, &(current_state->plane2colour));
    indent_newline();
    vrml_node ("geometry IndexedFaceSet");
    vrml_s_newline ("creaseAngle 1.6");

    indent_string ("coord USE");
    vrml_s_newline (def_name->string);

    vrml_list ("coordIndex");
    for (slot = 0; slot < helix_segment_count - 1; slot++) {
      base = 2 * slot;
      vrml_tri_indices (base, 0, 2, 1);
      vrml_tri_indices (base, 1, 2, 3);
    }
    vrml_finish_list();
    polygon_count += 2 * helix_segment_count;
  }

  vrml_finish_node();
  vrml_finish_node();

  if (def_name) ds_delete (def_name);
}


/*------------------------------------------------------------*/
void
vrml_label (vector3 *p, char *label, colour *c)
{
  assert (p);
  assert (label);
  assert (*label);

  indent_newline();
  vrml_node ("Label");
  indent_string ("p ");
  vrml_v3 (p);
  indent_string ("c");
  vrml_s_quoted (label);
  if (v3_length (&(current_state->labeloffset)) >= 0.001) {
    indent_string (" o ");
    vrml_v3 (&(current_state->labeloffset));
  }
  if (c) {
    output_dc (c);
  } else {
    output_dc (&(current_state->linecolour));
  }
  output_ec (&(current_state->emissivecolour));
  output_sc (&(current_state->specularcolour));
  output_sh (current_state->shininess);
  output_tr (current_state->transparency);
  indent_string ("sz");
  vrml_g3 (current_state->labelsize / 20.0);
  vrml_finish_node();

  label_count++;
}


/*------------------------------------------------------------*/
void
vrml_line (boolean polylines)
{
  int slot;

  if (line_segment_count < 2) return;

  indent_newline();
  vrml_comment ("MolScript: line, trace, bonds, coil or turn");
  vrml_node ("Shape");
  output_appearance (TRUE, &(line_segments[0].c));
  indent_newline();
  vrml_node ("geometry IndexedLineSet");

  indent_string ("coord");
  vrml_node ("Coordinate");
  vrml_list ("point");
  for (slot = 0; slot < line_segment_count; slot++)
    vrml_v3 (&(line_segments[slot].p));
  vrml_finish_list();
  vrml_finish_node();

  indent_newline();
  vrml_list ("coordIndex");
  if (polylines) {
    vrml_i (0);
    for (slot = 1; slot < line_segment_count; slot++) {
      if (line_segments[slot].new) vrml_i (-1);
      vrml_i (slot);
    }
  } else {
    for (slot = 0; slot < line_segment_count; slot += 2) {
      vrml_i (slot);
      vrml_i (slot + 1);
      vrml_i (-1);
    }
  }
  vrml_finish_list();

  if (current_state->colourparts) {

    indent_newline();
    vrml_s_newline ("colorPerVertex FALSE");
    vrml_node ("color Color");
    vrml_list ("color");

    colour_cache_init();
    if (polylines) {
      for (slot = 0; slot < line_segment_count; slot++) {
	if (line_segments[slot].new) {
	  colour_cache_add (&(line_segments[slot].c));
	}
      }
    } else {
      for (slot = 0; slot < line_segment_count; slot += 2) {
	colour_cache_add (&(line_segments[slot].c));
      }
    }
    vrml_finish_list();
    vrml_finish_node();

    indent_newline();
    vrml_list ("colorIndex");
    colour_cache_index_init();

    if (polylines) {
      for (slot = 0; slot < line_segment_count; slot++) {
	if (line_segments[slot].new) {
	  vrml_i (colour_cache_index (&(line_segments[slot].c)));
	}
      }
    } else {
      for (slot = 0; slot < line_segment_count; slot += 2) {
	vrml_i (colour_cache_index (&(line_segments[slot].c)));
      }
    }
    vrml_finish_list();
  }
  vrml_finish_node();
  vrml_finish_node();

  line_count += line_segment_count;
}


/*------------------------------------------------------------*/
void
vrml_sphere (at3d *at, double radius)
{
  assert (at);
  assert (radius > 0.0);

  indent_newline();
  vrml_node ("Ball");
  indent_string ("p ");
  vrml_v3 (&(at->xyz));
  indent_string ("rad");
  vrml_g3 (radius);
  output_dc (&(at->colour));
  output_ec (&(current_state->emissivecolour));
  output_sc (&(current_state->specularcolour));
  output_sh (current_state->shininess);
  output_tr (current_state->transparency);
  vrml_finish_node();

  sphere_count++;
}


/*------------------------------------------------------------*/
void
vrml_stick (vector3 *v1, vector3 *v2, double r1, double r2, colour *c)
{
  vector3 dir, rot, middle;
  double angle;

  assert (v1);
  assert (v2);
  assert (v3_distance (v1, v2) > 0.0);

  v3_difference (&dir, v1, v2);
  angle = v3_angle (&dir, &yaxis);
  if (angle > 0.0) {
    v3_cross_product (&rot, &yaxis, &dir);
    v3_normalize (&rot);
  } else {
    rot = yaxis;
  }
  v3_middle (&middle, v1, v2);

  indent_newline();
  vrml_node ("Stick");
  indent_string ("p ");
  vrml_v3_g (&middle);		/* vrml_v3 causes gaps between half-sticks */
  output_rot (&rot, angle);
  output_scale (current_state->stickradius,
		0.502 * v3_length (&dir), /* kludge to avoid glitches */
		current_state->stickradius);
  if (c) {
    output_dc (c);
  } else {
    output_dc (&(current_state->planecolour));
  }
  output_ec (&(current_state->emissivecolour));
  output_sc (&(current_state->specularcolour));
  output_sh (current_state->shininess);
  output_tr (current_state->transparency);
  vrml_finish_node();

  polygon_count += STICK_SEGMENTS;
}


/*------------------------------------------------------------*/
void
vrml_strand (void)
{
  int slot, base, thickness, cycle, cycles;
  strand_segment *ss;
  dynstring *def_name = NULL;

  thickness = current_state->strandthickness >= 0.01;

  indent_newline();
  if (current_state->colourparts || !thickness) {
    vrml_comment ("MolScript: strand");
  } else {
    vrml_comment ("MolScript: strand main faces");
  }
  vrml_node ("Shape");
  output_appearance (FALSE, &(current_state->planecolour));
  indent_newline();
  vrml_node ("geometry IndexedFaceSet");
  vrml_s_newline ("creaseAngle 1");
  if (! thickness) vrml_s_newline ("solid FALSE");

  indent_string ("coord");
  if (! current_state->colourparts && thickness) {
    def_name = ds_create ("_");
    ds_cat_int (def_name, def_count++);
    vrml_def (def_name->string);
  }
  vrml_node ("Coordinate");
  vrml_list ("point");
  if (thickness) {		/* strand main face */
    for (slot = 0; slot < strand_segment_count - 1; slot++) { /* upper */
      ss = strand_segments + slot;
      vrml_v3 (&(ss->p1));
      vrml_v3 (&(ss->p2));
      vrml_v3 (&(ss->p3));
      vrml_v3 (&(ss->p4));
    }
    ss = strand_segments + strand_segment_count - 1;
    vrml_v3 (&(ss->p1));
    vrml_v3 (&(ss->p2));
  } else {
    for (slot = 0; slot < strand_segment_count - 1; slot++) { /* lower */
      ss = strand_segments + slot;
      vrml_v3 (&(ss->p1));
      vrml_v3 (&(ss->p3));
    }
    ss = strand_segments + strand_segment_count - 1;
    vrml_v3 (&(ss->p1));
  }
  vrml_finish_list();
  vrml_finish_node();

  indent_newline();
  vrml_list ("coordIndex");
  if (thickness) {		/* strand face, thick */
    for (slot = 0; slot < strand_segment_count - 4; slot++) { /* upper */
      base = 4 * slot;
      vrml_tri_indices (base, 0, 3, 4);
      vrml_tri_indices (base, 3, 7, 4);
    }
    polygon_count += 2 * (strand_segment_count - 4);

    base += 4;			/* arrow head upper */
    vrml_tri_indices (base, 0, 8, 4);
    vrml_tri_indices (base, 0, 3, 8);
    vrml_tri_indices (base, 3, 11, 8);
    vrml_tri_indices (base, 3, 7, 11);
    vrml_tri_indices (base, 8, 11, 12);
    polygon_count += 5;

    for (slot = 0; slot < strand_segment_count - 4; slot++) { /* lower */
      base = 4 * slot;
      vrml_tri_indices (base, 2, 1, 5);
      vrml_tri_indices (base, 5, 6, 2);
    }
    polygon_count += 2 * (strand_segment_count - 4);

    base += 4;			/* arrow head lower */
    vrml_tri_indices (base, 1, 5, 9);
    vrml_tri_indices (base, 2, 1, 9);
    vrml_tri_indices (base, 2, 9, 10);
    vrml_tri_indices (base, 2, 10, 6);
    vrml_tri_indices (base, 10, 9, 13);
    polygon_count += 5;

    if (def_name) {
      vrml_finish_list();	/* finish polygons for main faces */

      vrml_finish_node();	/* finish Shape node for main faces */
      vrml_finish_node();

      indent_newline();
      vrml_comment ("MolScript: strand side faces");
      vrml_node ("Shape");
      output_appearance (FALSE, &(current_state->plane2colour));
      indent_newline();
      vrml_node ("geometry IndexedFaceSet");
      vrml_s_newline ("creaseAngle 0.79");

      indent_string ("coord USE");
      vrml_s_newline (def_name->string);

      vrml_list ("coordIndex");	/* begin polygons for side faces */
    }

    vrml_quad_indices (0, 0, 1, 2, 3); /* strand base */
    polygon_count++;

    for (slot = 0; slot < strand_segment_count - 4; slot++) { /* side left */
      base = 4 * slot;
      vrml_tri_indices (base, 1, 0, 5);
      vrml_tri_indices (base, 0, 4, 5);
    }
    polygon_count += 2 * (strand_segment_count - 4);

    for (slot = 0; slot < strand_segment_count - 4; slot++) { /* side right */
      base = 4 * slot;
      vrml_tri_indices (base, 2, 6, 3);
      vrml_tri_indices (base, 3, 6, 7);
    }
    polygon_count += 2 * (strand_segment_count - 4);

    base += 4;
    vrml_quad_indices (base, 4, 5, 1, 0); /* arrow base */
    vrml_quad_indices (base, 3, 2, 6, 7);
    polygon_count += 2;

    base += 4;
    vrml_tri_indices (base, 1, 0, 5); /* arrow side left 1 */
    vrml_tri_indices (base, 0, 4, 5);
    vrml_tri_indices (base, 2, 6, 3); /* arrow side right 1 */
    vrml_tri_indices (base, 3, 6, 7);
    polygon_count += 4;

    base += 4;
    vrml_tri_indices (base, 1, 0, 5); /* arrow side left 2 */
    vrml_tri_indices (base, 0, 4, 5);
    vrml_tri_indices (base, 2, 5, 3); /* arrow side right 2 */
    vrml_tri_indices (base, 3, 5, 4);
    polygon_count += 4;

  } else {			/* strand face, thin */

    for (slot = 0; slot < strand_segment_count - 4; slot++) {
      base = 2 * slot;
      vrml_tri_indices (base, 0, 1, 2);
      vrml_tri_indices (base, 1, 3, 2);
    }
    polygon_count += 2 * (strand_segment_count - 4);

    base += 2;
    vrml_tri_indices (base, 0, 4, 2);
    vrml_tri_indices (base, 0, 1, 4);
    vrml_tri_indices (base, 1, 5, 4);
    vrml_tri_indices (base, 1, 3, 5);
    vrml_tri_indices (base, 4, 5, 6);
    polygon_count += 5;
  }
  vrml_finish_list();

  if (current_state->colourparts) {
    colour rgb;

    indent_newline();
    vrml_s_newline ("colorPerVertex FALSE");
    vrml_node ("color Color");
    vrml_list ("color");

    colour_to_rgb (&(strand_segments[0].c));
    rgb = strand_segments[0].c;
    vrml_rgb_colour (&rgb);

    for (slot = 1; slot < strand_segment_count - 1; slot++) {
      ss = strand_segments + slot;

      colour_to_rgb (&(strand_segments[slot].c));
      if (colour_unequal (&rgb, &(ss->c))) {
	rgb = ss->c;
	vrml_rgb_colour (&rgb);
	colour_count++;
      }
    }
    vrml_finish_list();
    vrml_finish_node();

    indent_newline();
    vrml_list ("colorIndex");

    if (thickness) cycles = 2; else cycles = 1;

    for (cycle = 0; cycle < cycles; cycle++) {
      rgb = strand_segments[0].c;
      base = 0;
      vrml_i (base);
      vrml_i (base);

      for (slot = 1; slot < strand_segment_count - 4; slot++) {
	ss = strand_segments + slot;
	if (colour_unequal (&rgb, &(ss->c))) {
	  rgb = ss->c;
	  base++;
	}
	vrml_i (base);
	vrml_i (base);
      }

      vrml_i (base);		/* arrow head first part */
      vrml_i (base);
      vrml_i (base);
      vrml_i (base);
      if (colour_unequal (&rgb, &(strand_segments[strand_segment_count-2].c))){
	rgb = strand_segments[strand_segment_count-2].c;
	base++;
      }
      vrml_i (base);		/* arrow head last part */

    }

    if (thickness) {		/* strand sides */
      rgb = strand_segments[0].c;
      base = 0;
      vrml_i (base);		/* strand base */

      for (slot = 0; slot < strand_segment_count - 4; slot++) { /* side right*/
	if (colour_unequal (&rgb, &(strand_segments[slot].c))) {
	  rgb = strand_segments[slot].c;
	  base++;
	}
	vrml_i (base);
	vrml_i (base);
      }

      rgb = strand_segments[0].c;
      base = 0;

      for (slot = 0; slot < strand_segment_count - 4; slot++) { /* side left */
	if (colour_unequal (&rgb, &(strand_segments[slot].c))) {
	  rgb = strand_segments[slot].c;
	  base++;
	}
	vrml_i (base);
	vrml_i (base);
      }

      vrml_i (base);		/* arrow base */
      vrml_i (base);

      vrml_i (base);		/* arrow side left 1 */
      vrml_i (base);
      vrml_i (base);		/* arrow side right 1 */
      vrml_i (base);

      if (colour_unequal (&rgb, &(strand_segments[strand_segment_count-2].c))){
	rgb = strand_segments[strand_segment_count-2].c;
	base++;
      }
      vrml_i (base);		/* arrow side left 2 */
      vrml_i (base);
      vrml_i (base);		/* arrow side right 2 */
      vrml_i (base);
    }

    vrml_finish_list();
  }

  vrml_finish_node();
  vrml_finish_node();

  if (def_name) ds_delete (def_name);
}


/*------------------------------------------------------------*/
void
vrml_strand2 (void)
{
  int slot, base, thickness, cycle, cycles;
  strand_segment *ss;

  thickness = current_state->strandthickness >= 0.01;

  indent_newline();
  vrml_comment ("MolScript: strand");
  vrml_node ("Shape");
  output_appearance (FALSE, &(current_state->planecolour));
  indent_newline();
  vrml_node ("geometry IndexedFaceSet");
  vrml_s_newline ("creaseAngle 0.79");
  if (!thickness) vrml_s_newline ("solid FALSE");

  indent_string ("coord");
  vrml_node ("Coordinate");
  vrml_list ("point");
  if (thickness) {		/* strand main face */
    for (slot = 0; slot < strand_segment_count - 1; slot++) { /* upper */
      ss = strand_segments + slot;
      vrml_v3 (&(ss->p1));
      vrml_v3 (&(ss->p2));
      vrml_v3 (&(ss->p3));
      vrml_v3 (&(ss->p4));
    }
    ss = strand_segments + strand_segment_count - 1;
    vrml_v3 (&(ss->p1));
    vrml_v3 (&(ss->p2));
  } else {
    for (slot = 0; slot < strand_segment_count - 1; slot++) { /* lower */
      ss = strand_segments + slot;
      vrml_v3 (&(ss->p1));
      vrml_v3 (&(ss->p3));
    }
    ss = strand_segments + strand_segment_count - 1;
    vrml_v3 (&(ss->p1));
  }
  vrml_finish_list();
  vrml_finish_node();

  indent_newline();
  vrml_list ("coordIndex");
  if (thickness) {		/* strand face, thick */
    for (slot = 0; slot < strand_segment_count - 4; slot++) { /* upper */
      base = 4 * slot;
      vrml_tri_indices (base, 0, 3, 4);
      vrml_tri_indices (base, 3, 7, 4);
    }
    polygon_count += 2 * (strand_segment_count - 4);

    base += 4;			/* arrow head upper */
    vrml_tri_indices (base, 0, 8, 4);
    vrml_tri_indices (base, 0, 3, 8);
    vrml_tri_indices (base, 3, 11, 8);
    vrml_tri_indices (base, 3, 7, 11);
    vrml_tri_indices (base, 8, 11, 12);
    polygon_count += 5;

    for (slot = 0; slot < strand_segment_count - 4; slot++) { /* lower */
      base = 4 * slot;
      vrml_tri_indices (base, 2, 1, 5);
      vrml_tri_indices (base, 5, 6, 2);
    }
    polygon_count += 2 * (strand_segment_count - 4);

    base += 4;			/* arrow head lower */
    vrml_tri_indices (base, 1, 5, 9);
    vrml_tri_indices (base, 2, 1, 9);
    vrml_tri_indices (base, 2, 9, 10);
    vrml_tri_indices (base, 2, 10, 6);
    vrml_tri_indices (base, 10, 9, 13);
    polygon_count += 5;

    vrml_quad_indices (0, 0, 1, 2, 3); /* strand base */
    polygon_count++;

    for (slot = 0; slot < strand_segment_count - 4; slot++) { /* side left */
      base = 4 * slot;
      vrml_tri_indices (base, 1, 0, 5);
      vrml_tri_indices (base, 0, 4, 5);
    }
    polygon_count += 2 * (strand_segment_count - 4);

    for (slot = 0; slot < strand_segment_count - 4; slot++) { /* side right */
      base = 4 * slot;
      vrml_tri_indices (base, 2, 6, 3);
      vrml_tri_indices (base, 3, 6, 7);
    }
    polygon_count += 2 * (strand_segment_count - 4);

    base += 4;
    vrml_quad_indices (base, 4, 5, 1, 0); /* arrow base */
    vrml_quad_indices (base, 3, 2, 6, 7);
    polygon_count += 2;

    base += 4;
    vrml_tri_indices (base, 1, 0, 5); /* arrow side left 1 */
    vrml_tri_indices (base, 0, 4, 5);
    vrml_tri_indices (base, 2, 6, 3); /* arrow side right 1 */
    vrml_tri_indices (base, 3, 6, 7);
    polygon_count += 4;

    base += 4;
    vrml_tri_indices (base, 1, 0, 5); /* arrow side left 2 */
    vrml_tri_indices (base, 0, 4, 5);
    vrml_tri_indices (base, 2, 5, 3); /* arrow side right 2 */
    vrml_tri_indices (base, 3, 5, 4);
    polygon_count += 4;

  } else {			/* strand face, thin */

    for (slot = 0; slot < strand_segment_count - 4; slot++) {
      base = 2 * slot;
      vrml_tri_indices (base, 0, 1, 2);
      vrml_tri_indices (base, 1, 3, 2);
    }
    polygon_count += 2 * (strand_segment_count - 4);

    base += 2;
    vrml_tri_indices (base, 0, 4, 2);
    vrml_tri_indices (base, 0, 1, 4);
    vrml_tri_indices (base, 1, 5, 4);
    vrml_tri_indices (base, 1, 3, 5);
    vrml_tri_indices (base, 4, 5, 6);
    polygon_count += 5;
  }
  vrml_finish_list();

  if (current_state->colourparts) {
    colour rgb;

    indent_newline();
    vrml_s_newline ("colorPerVertex FALSE");
    vrml_node ("color Color");
    vrml_list ("color");

    colour_to_rgb (&(strand_segments[0].c));
    rgb = strand_segments[0].c;
    vrml_rgb_colour (&rgb);

    for (slot = 1; slot < strand_segment_count; slot++) {
      ss = strand_segments + slot;

      colour_to_rgb (&(strand_segments[slot].c));
      if (colour_unequal (&rgb, &(ss->c))) {
	rgb = ss->c;
	vrml_rgb_colour (&rgb);
	colour_count++;
      }
    }
    vrml_finish_list();
    vrml_finish_node();

    indent_newline();
    vrml_list ("colorIndex");

    if (thickness) cycles = 2; else cycles = 1;

    for (cycle = 0; cycle < cycles; cycle++) {
      rgb = strand_segments[0].c;
      base = 0;
      vrml_i (base);
      vrml_i (base);

      for (slot = 1; slot < strand_segment_count - 4; slot++) {
	ss = strand_segments + slot;
	if (colour_unequal (&rgb, &(ss->c))) {
	  rgb = ss->c;
	  base++;
	}
	vrml_i (base);
	vrml_i (base);
      }

      vrml_i (base);		/* arrow head first part */
      vrml_i (base);
      vrml_i (base);
      vrml_i (base);
      if (colour_unequal (&rgb, &(strand_segments[strand_segment_count-2].c))){
	rgb = strand_segments[strand_segment_count-2].c;
	base++;
      }
      vrml_i (base);		/* arrow head last part */

    }

    if (thickness) {		/* strand sides */
      rgb = strand_segments[0].c;
      base = 0;
      vrml_i (base);		/* strand base */

      for (slot = 0; slot < strand_segment_count - 4; slot++) { /* side right*/
	if (colour_unequal (&rgb, &(strand_segments[slot].c))) {
	  rgb = strand_segments[slot].c;
	  base++;
	}
	vrml_i (base);
	vrml_i (base);
      }

      rgb = strand_segments[0].c;
      base = 0;

      for (slot = 0; slot < strand_segment_count - 4; slot++) { /* side left */
	if (colour_unequal (&rgb, &(strand_segments[slot].c))) {
	  rgb = strand_segments[slot].c;
	  base++;
	}
	vrml_i (base);
	vrml_i (base);
      }

      vrml_i (base);		/* arrow base */
      vrml_i (base);

      vrml_i (base);		/* arrow side left 1 */
      vrml_i (base);
      vrml_i (base);		/* arrow side right 1 */
      vrml_i (base);

      if (colour_unequal (&rgb, &(strand_segments[strand_segment_count-2].c))){
	rgb = strand_segments[strand_segment_count-2].c;
	base++;
      }
      vrml_i (base);		/* arrow side left 2 */
      vrml_i (base);
      vrml_i (base);		/* arrow side right 2 */
      vrml_i (base);
    }

    vrml_finish_list();
  }
  vrml_finish_node();
  vrml_finish_node();
}


/*------------------------------------------------------------*/
void
vrml_object (int code, vector3 *triplets, int count)
{
  int slot;
  vector3 *v;
  colour rgb = {COLOUR_RGB, 1.0, 1.0, 1.0};

  assert (triplets);
  assert (count > 0);

  indent_newline();
  vrml_node ("Shape");

  switch (code) {

  case OBJ_POINTS:
    vrml_comment ("MolScript: P object");
    output_appearance (TRUE, &(current_state->linecolour));
    indent_newline();
    vrml_node ("geometry PointSet");
    vrml_node ("coord Coordinate");
    vrml_list ("point");
    for (slot = 0; slot < count; slot++) vrml_v3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    vrml_finish_node();
    point_count += count;
    break;

  case OBJ_POINTS_COLOURS:
    vrml_comment ("MolScript: PC object");
    output_appearance (TRUE, &(current_state->linecolour));
    indent_newline();
    vrml_node ("geometry PointSet");
    vrml_node ("coord Coordinate");
    vrml_list ("point");
    for (slot = 0; slot < count; slot += 2) vrml_v3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_node ("color Color");
    vrml_list ("color");
    for (slot = 1; slot < count; slot += 2) {
      v = triplets + slot;
      rgb.x = v->x;
      rgb.y = v->y;
      rgb.z = v->z;
      vrml_rgb_colour (&rgb);
    }
    vrml_finish_list();
    vrml_finish_node();
    vrml_finish_node();
    vrml_finish_node();
    point_count += count;
    break;

  case OBJ_LINES:
    vrml_comment ("MolScript: L object");
    output_appearance (TRUE, &(current_state->linecolour));
    indent_newline();
    vrml_node ("geometry IndexedLineSet");
    vrml_node ("coord Coordinate");
    vrml_list ("point");
    for (slot = 0; slot < count; slot++) vrml_v3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_list ("coordIndex");
    for (slot = 0; slot < count; slot++) vrml_i (slot);
    vrml_finish_list();
    vrml_finish_node();
    line_count += count - 1;
    break;

  case OBJ_LINES_COLOURS:
    vrml_comment ("MolScript: LC object");
    output_appearance (TRUE, &(current_state->linecolour));
    indent_newline();
    vrml_node ("geometry IndexedLineSet");
    vrml_node ("coord Coordinate");
    vrml_list ("point");
    for (slot = 0; slot < count; slot += 2) vrml_v3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_list ("coordIndex");
    for (slot = 0; slot < count / 2; slot++) vrml_i (slot);
    vrml_finish_list();
    indent_newline();
    vrml_node ("color Color");
    vrml_list ("color");
    for (slot = 1; slot < count; slot += 2) {
      v = triplets + slot;
      rgb.x = v->x;
      rgb.y = v->y;
      rgb.z = v->z;
      vrml_rgb_colour (&rgb);
    }
    vrml_finish_list();
    vrml_finish_node();
    vrml_finish_node();
    line_count += count - 1;
    break;

  case OBJ_TRIANGLES:
    vrml_comment ("MolScript: T object");
    output_appearance (FALSE, &(current_state->planecolour));
    indent_newline();
    vrml_node ("geometry IndexedFaceSet");
    vrml_s_newline ("solid FALSE");
    vrml_node ("coord Coordinate");
    vrml_list ("point");
    for (slot = 0; slot < count; slot++) vrml_v3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_list ("coordIndex");
    for (slot = 0; slot < count; slot += 3) vrml_tri_indices (slot, 0, 1, 2);
    vrml_finish_list();
    vrml_finish_node();
    polygon_count += count / 3;
    break;

  case OBJ_TRIANGLES_COLOURS:
    vrml_comment ("MolScript: TC object");
    output_appearance (FALSE, &(current_state->planecolour));
    indent_newline();
    vrml_node ("geometry IndexedFaceSet");
    vrml_s_newline ("solid FALSE");
    vrml_node ("coord Coordinate");
    vrml_list ("point");
    for (slot = 0; slot < count; slot += 2) vrml_v3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_list ("coordIndex");
    for (slot = 0; slot < count/2; slot += 3) vrml_tri_indices (slot, 0, 1, 2);
    vrml_finish_list();
    indent_newline();
    vrml_node ("color Color");
    vrml_list ("color");
    for (slot = 1; slot < count; slot += 2) {
      v = triplets + slot;
      rgb.x = v->x;
      rgb.y = v->y;
      rgb.z = v->z;
      vrml_rgb_colour (&rgb);
    }
    vrml_finish_list();
    vrml_finish_node();
    vrml_finish_node();
    polygon_count += count / 6;
    break;

  case OBJ_TRIANGLES_NORMALS:
    vrml_comment ("MolScript: TN object");
    output_appearance (FALSE, &(current_state->planecolour));
    indent_newline();
    vrml_node ("geometry IndexedFaceSet");
    vrml_s_newline ("solid FALSE");
    vrml_node ("coord Coordinate");
    vrml_list ("point");
    for (slot = 0; slot < count; slot += 2) vrml_v3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_list ("coordIndex");
    for (slot = 0; slot < count/2; slot += 3) vrml_tri_indices (slot, 0, 1, 2);
    vrml_finish_list();
    indent_newline();
    vrml_node ("normal Normal");
    vrml_list ("vector");
    for (slot = 1; slot < count; slot += 2) vrml_v3_g3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    vrml_finish_node();
    polygon_count += count / 6;
    break;

  case OBJ_TRIANGLES_NORMALS_COLOURS:
    vrml_comment ("MolScript: TNC object");
    output_appearance (FALSE, &(current_state->planecolour));
    indent_newline();
    vrml_node ("geometry IndexedFaceSet");
    vrml_s_newline ("solid FALSE");
    vrml_node ("coord Coordinate");
    vrml_list ("point");
    for (slot = 0; slot < count; slot += 3) vrml_v3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_list ("coordIndex");
    for (slot = 0; slot < count/3; slot += 3) vrml_tri_indices (slot, 0, 1, 2);
    vrml_finish_list();
    indent_newline();
    vrml_node ("normal Normal");
    vrml_list ("vector");
    for (slot = 1; slot < count; slot += 3) vrml_v3_g3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_node ("color Color");
    vrml_list ("color");
    for (slot = 2; slot < count; slot += 3) {
      v = triplets + slot;
      rgb.x = v->x;
      rgb.y = v->y;
      rgb.z = v->z;
      vrml_rgb_colour (&rgb);
    }
    vrml_finish_list();
    vrml_finish_node();
    vrml_finish_node();
    polygon_count += count / 9;
    break;

  case OBJ_STRIP:
    vrml_comment ("MolScript: S object");
    output_appearance (FALSE, &(current_state->planecolour));
    indent_newline();
    vrml_node ("geometry IndexedFaceSet");
    vrml_s_newline ("solid FALSE");
    vrml_node ("coord Coordinate");
    vrml_list ("point");
    for (slot = 0; slot < count; slot++) vrml_v3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_list ("coordIndex");
    for (slot = 0; slot < count - 2; slot++) vrml_tri_indices (slot, 0, 1, 2);
    vrml_finish_list();
    vrml_finish_node();
    polygon_count += count - 2;
    break;

  case OBJ_STRIP_COLOURS:
    vrml_comment ("MolScript: SC object");
    output_appearance (FALSE, &(current_state->planecolour));
    indent_newline();
    vrml_node ("geometry IndexedFaceSet");
    vrml_s_newline ("solid FALSE");
    vrml_s_newline ("colorPerVertex FALSE");
    vrml_node ("coord Coordinate");
    vrml_list ("point");
    for (slot = 0; slot < count; slot += 2) vrml_v3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_list ("coordIndex");
    for (slot = 0; slot < count/2 - 2; slot++) vrml_tri_indices (slot, 0,1,2);
    vrml_finish_list();
    indent_newline();
    vrml_node ("color Color");
    vrml_list ("color");
    for (slot = 5; slot < count; slot += 2) {
      v = triplets + slot;
      rgb.x = v->x;
      rgb.y = v->y;
      rgb.z = v->z;
      vrml_rgb_colour (&rgb);
    }
    vrml_finish_list();
    vrml_finish_node();
    vrml_finish_node();
    polygon_count += count / 2 - 2;
    break;

  case OBJ_STRIP_NORMALS:
    vrml_comment ("MolScript: SN object");
    output_appearance (FALSE, &(current_state->planecolour));
    indent_newline();
    vrml_node ("geometry IndexedFaceSet");
    vrml_s_newline ("solid FALSE");
    vrml_s_newline ("colorPerVertex FALSE");
    vrml_node ("coord Coordinate");
    vrml_list ("point");
    for (slot = 0; slot < count; slot += 2) vrml_v3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_list ("coordIndex");
    for (slot = 0; slot < count /2 - 2; slot++) {
      if (slot % 2) {
	vrml_tri_indices (slot, 1,0,2);
      } else {
	vrml_tri_indices (slot, 0,1,2);
      }
    }
    vrml_finish_list();
    indent_newline();
    vrml_node ("normal Normal");
    vrml_list ("vector");
    for (slot = 1; slot < count; slot += 2) vrml_v3_g3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    vrml_finish_node();
    polygon_count += count / 2 - 2;
    break;

  case OBJ_STRIP_NORMALS_COLOURS:
    vrml_comment ("MolScript: SNC object");
    output_appearance (FALSE, &(current_state->planecolour));
    indent_newline();
    vrml_node ("geometry IndexedFaceSet");
    vrml_s_newline ("solid FALSE");
    vrml_node ("coord Coordinate");
    vrml_list ("point");
    for (slot = 0; slot < count; slot += 3) vrml_v3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_list ("coordIndex");
    for (slot = 0; slot < count / 3 - 2; slot++) {
      if (slot % 2) {
	vrml_tri_indices (slot, 1,0,2);
      } else {
	vrml_tri_indices (slot, 0,1,2);
      }
    }
    vrml_finish_list();
    indent_newline();
    vrml_node ("normal Normal");
    vrml_list ("vector");
    for (slot = 1; slot < count; slot += 3) vrml_v3_g3 (triplets + slot);
    vrml_finish_list();
    vrml_finish_node();
    indent_newline();
    vrml_node ("color Color");
    vrml_list ("color");
    for (slot = 2; slot < count; slot += 3) {
      v = triplets + slot;
      rgb.x = v->x;
      rgb.y = v->y;
      rgb.z = v->z;
      vrml_rgb_colour (&rgb);
    }
    vrml_finish_list();
    vrml_finish_node();
    vrml_finish_node();
    polygon_count += count / 3 - 2;
    break;

  }

  vrml_finish_node();
}
