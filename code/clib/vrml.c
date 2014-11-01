/*
   VRML 2.0 output routines.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
     4-Mar-1997  split out of MolScript
     3-Apr-1997  added blank before '{' and '[', for some browsers
    14-Feb-1998  output file needs explicit initialization
     8-Jun-1998  allow double-quote '"' in vrml_s_quoted output
    10-Jun-1998  mod's for hgen
    24-Aug-1998  split out indent functions into separate package
*/

#include "vrml.h"

/* public ====================
#include <stdio.h>

#include <vector3.h>
#include <colour.h>
#include <indent.h>
==================== public */

#include <assert.h>
#include <string.h>
#include <math.h>

#include <angle.h>


/*============================================================*/
static colour rgb;
static const colour default_diffusecolour = {COLOUR_RGB, 0.8, 0.8, 0.8};
static const colour default_emissivecolour = {COLOUR_RGB, 0.0, 0.0, 0.0};
static const colour default_specularcolour = {COLOUR_RGB, 0.0, 0.0, 0.0};
static const colour white_colour = {COLOUR_RGB, 1.0, 1.0, 1.0};


/*------------------------------------------------------------*/
void
vrml_initialize (void)
     /*
       Initialize the VRML output file to stdout, and set standard
       indenting values.
     */
{
  indent_initialize (stdout);
}


/*------------------------------------------------------------*/
int
vrml_header (void)
     /*
       Write the VRML header line to the file, and return the number
       of written characters (negative if error).
     */
{
  /* pre */
  assert (indent_valid_state());

  indent_needs_blank = FALSE;
  return fprintf (indent_file, "#VRML V2.0 utf8\n");
}


/*------------------------------------------------------------*/
void
vrml_comment (char *comment)
{
  /* pre */
  assert (indent_valid_state());
  assert (comment);
  assert (*comment);

  if (indent_flag) {
    indent_newline_conditional();
    INDENT_FPRINTF (indent_file, "# %s", comment);
    indent_newline();
  }
}


/*------------------------------------------------------------*/
void
vrml_worldinfo (char *title, char *info)
{
  /* pre */
  assert (indent_valid_state());

  vrml_node ("WorldInfo");
  if (title) {
    INDENT_FPRINTF (indent_file, "title \"%s\"", title);
  }
  if (info) {
    if (title) indent_newline();
    vrml_list ("info");
    vrml_s_quoted (info);
    vrml_finish_list();
  }
  vrml_finish_node();
}


/*------------------------------------------------------------*/
void
vrml_navigationinfo (double speed, char *type)
{
  /* pre */
  assert (speed > 0.0);

  indent_newline();
  vrml_node ("NavigationInfo");
  if (speed != 1.0) {
    indent_string ("speed");
    vrml_g (speed);
  }
  if (type) {
    indent_newline_conditional();
    indent_string ("type");
    indent_string (type);
  }
  vrml_finish_node();
}


/*------------------------------------------------------------*/
void
vrml_viewpoint (vector3 *pos, vector3 *ori, double rot,
		double fov, char *descr)
{
  /* pre */
  assert (pos);
  assert (fov >= 0.0);

  indent_newline();
  vrml_node ("Viewpoint");
  indent_string ("position");
  vrml_v3 (pos);
  if (ori) {
    indent_newline();
    indent_string ("orientation");
    vrml_v3_g (ori);
    vrml_g (rot);
  }
  if (fov > 0.0) {
    indent_newline();
    indent_string ("fieldOfView");
    vrml_g (fov);
  }
  if (descr) {
    indent_newline();
    indent_string ("description");
    vrml_s_quoted (descr);
  }
  vrml_finish_node();
}


/*------------------------------------------------------------*/
void
vrml_fog (int type_exp, double r, colour *c)
{
  /* pre */
  assert (r > 0.0);
  assert (c);

  indent_newline();
  vrml_node ("Fog");
  indent_string ("fogType");
  if (type_exp) {
    vrml_s_quoted ("EXPONENTIAL");
  } else {
    vrml_s_quoted ("LINEAR");
  }
  indent_newline();
  indent_string ("visibilityRange");
  vrml_g (r);
  indent_newline();
  indent_string ("color ");		/* last blank char needed */
  vrml_colour (c);
  vrml_finish_node();
}


/*------------------------------------------------------------*/
void
vrml_s_newline (char *str)
{
  /* pre */
  assert (indent_valid_state());
  assert (str);

  indent_check_buflen (strlen (str));
  indent_blank();
  INDENT_FPRINTF (indent_file, "%s", str);
  indent_newline();
}


/*------------------------------------------------------------*/
void
vrml_s_quoted (char *str)
{
  /* pre */
  assert (indent_valid_state());
  assert (str);

  indent_check_buflen (strlen (str) + 2);
  indent_blank();		/* strictly not needed */
  indent_putchar ('"');
  for ( ; *str; str++) {
    if (*str == '"') indent_putchar ('\\');
    indent_putchar (*str);
  }
  indent_putchar ('"');
  indent_needs_blank = TRUE;	/* strictly not needed */
}


/*------------------------------------------------------------*/
void
vrml_i (int i)
{
  /* pre */
  assert (indent_valid_state());

  indent_check_buflen (1);
  indent_blank();
  INDENT_FPRINTF (indent_file, "%i", i);
  indent_needs_blank = TRUE;
}


/*------------------------------------------------------------*/
void
vrml_f (double d)
{
  /* pre */
  assert (indent_valid_state());

  indent_check_buflen (1);
  indent_blank();
  INDENT_FPRINTF (indent_file, "%f", d);
  indent_needs_blank = TRUE;
}


/*------------------------------------------------------------*/
void
vrml_g (double d)
{
  /* pre */
  assert (indent_valid_state());

  indent_check_buflen (1);
  indent_blank();
  INDENT_FPRINTF (indent_file, "%g", d);
  indent_needs_blank = TRUE;
}


/*------------------------------------------------------------*/
void
vrml_f2 (double d)
{
  /* pre */
  assert (indent_valid_state());

  indent_check_buflen (1);
  indent_blank();
  INDENT_FPRINTF (indent_file, "%.2f", d);
  indent_needs_blank = TRUE;
}


/*------------------------------------------------------------*/
void
vrml_g3 (double d)
{
  /* pre */
  assert (indent_valid_state());

  indent_check_buflen (1);
  indent_blank();
  INDENT_FPRINTF (indent_file, "%.3g", d);
  indent_needs_blank = TRUE;
}


/*------------------------------------------------------------*/
void
vrml_v3 (vector3 *v)
{
  /* pre */
  assert (indent_valid_state());
  assert (v);

  indent_check_buflen (5);
  if (indent_flag) {
    if (indent_needs_blank) {
      INDENT_FPRINTF (indent_file, ", %.2f %.2f %.2f", v->x, v->y, v->z);
    } else {
      INDENT_FPRINTF (indent_file, "%.2f %.2f %.2f", v->x, v->y, v->z);
    }
  } else {
    if (indent_needs_blank) {
      INDENT_FPRINTF (indent_file, " %.2f %.2f %.2f", v->x, v->y, v->z);
    } else {
      INDENT_FPRINTF (indent_file, "%.2f %.2f %.2f", v->x, v->y, v->z);
    }
  }
  indent_needs_blank = TRUE;
}


/*------------------------------------------------------------*/
void
vrml_v3_g (vector3 *v)
{
  /* pre */
  assert (indent_valid_state());
  assert (v);

  indent_check_buflen (5);
  if (indent_flag) {
    if (indent_needs_blank) {
      INDENT_FPRINTF (indent_file, ", %g %g %g", v->x, v->y, v->z);
    } else {
      INDENT_FPRINTF (indent_file, "%g %g %g", v->x, v->y, v->z);
    }
  } else {
    if (indent_needs_blank) {
      INDENT_FPRINTF (indent_file, " %g %g %g", v->x, v->y, v->z);
    } else {
      INDENT_FPRINTF (indent_file, "%g %g %g", v->x, v->y, v->z);
    }
  }
  indent_needs_blank = TRUE;
}


/*------------------------------------------------------------*/
void
vrml_v3_g3 (vector3 *v)
{
  /* pre */
  assert (indent_valid_state());
  assert (v);

  indent_check_buflen (5);
  if (indent_flag) {
    if (indent_needs_blank) {
      INDENT_FPRINTF (indent_file, ", %.3g %.3g %.3g", v->x, v->y, v->z);
    } else {
      INDENT_FPRINTF (indent_file, "%.3g %.3g %.3g", v->x, v->y, v->z);
    }
  } else {
    if (indent_needs_blank) {
      INDENT_FPRINTF (indent_file, " %.3g %.3g %.3g", v->x, v->y, v->z);
    } else {
      INDENT_FPRINTF (indent_file, "%.3g %.3g %.3g", v->x, v->y, v->z);
    }
  }
  indent_needs_blank = TRUE;
}


/*------------------------------------------------------------*/
void
vrml_colour (colour *c)
{
  /* pre */
  assert (indent_valid_state());
  assert (c);

  colour_copy_to_rgb (&rgb, c);
  vrml_rgb_colour (&rgb);
}


/*------------------------------------------------------------*/
void
vrml_rgb_colour (colour *c)
{
  /* pre */
  assert (indent_valid_state());
  assert (c);
  assert (c->spec == COLOUR_RGB);

  indent_check_buflen (5);
  if (indent_flag) {
    if (indent_needs_blank) {
      INDENT_FPRINTF (indent_file, ", %.2g %.2g %.2g", c->x, c->y, c->z);
    } else {
      INDENT_FPRINTF (indent_file, "%.2g %.2g %.2g", c->x, c->y, c->z);
    }
  } else {
    if (indent_needs_blank) {
      INDENT_FPRINTF (indent_file, " %.2g %.2g %.2g", c->x, c->y, c->z);
    } else {
      INDENT_FPRINTF (indent_file, "%.2g %.2g %.2g", c->x, c->y, c->z);
    }
  }
  indent_needs_blank = TRUE;
}


/*------------------------------------------------------------*/
void
vrml_tri_indices (int base, int i1, int i2, int i3)
{
  /* pre */
  assert (indent_valid_state());
  assert (base >= 0);
  assert (i1 >= 0);
  assert (i2 >= 0);
  assert (i3 >= 0);

  indent_check_buflen (8);
  indent_blank();
  INDENT_FPRINTF (indent_file, "%i %i %i -1", base + i1, base + i2, base + i3);
  indent_needs_blank = TRUE;
}


/*------------------------------------------------------------*/
void
vrml_quad_indices (int base, int i1, int i2, int i3, int i4)
{
  /* pre */
  assert (indent_valid_state());
  assert (base >= 0);
  assert (i1 >= 0);
  assert (i2 >= 0);
  assert (i3 >= 0);
  assert (i4 >= 0);

  indent_check_buflen (10);
  indent_blank();
  INDENT_FPRINTF (indent_file,
		  "%i %i %i %i -1", base+ i1, base+ i2, base+ i3, base+ i4);
  indent_needs_blank = TRUE;
}


/*------------------------------------------------------------*/
void
vrml_begin_node (void)
{
  /* pre */
  assert (indent_valid_state());

  indent_blank();		/* not strictly needed, but it looks nicer */
  indent_putchar ('{');
  indent_increment_level (TRUE);
  indent_newline();
}


/*------------------------------------------------------------*/
void
vrml_finish_node (void)
{
  /* pre */
  assert (indent_valid_state());

  indent_increment_level (FALSE);
  indent_newline();
  indent_putchar ('}');
  indent_needs_blank = FALSE;
}


/*------------------------------------------------------------*/
void
vrml_begin_list (void)
{
  /* pre */
  assert (indent_valid_state());

  indent_blank();		/* not strictly needed, but it looks nicer */
  indent_putchar ('[');
  indent_increment_level (TRUE);
  indent_newline();
}


/*------------------------------------------------------------*/
void
vrml_finish_list (void)
{
  /* pre */
  assert (indent_valid_state());

  indent_increment_level (FALSE);
  indent_newline();
  indent_putchar (']');
  indent_needs_blank = FALSE;
}


/*------------------------------------------------------------*/
boolean
vrml_valid_name (char *name)
     /*
       Is the given name string valid?
     */
{
  /* pre */
  assert (name);

  if (*name == '\0') return FALSE;
  if (strchr ("+-0123456789\"',.[]\\{}", *name) != NULL) return FALSE;
  if (*(name + 1) && (strpbrk (name + 1, "\"',.[]\\{}") != NULL)) return FALSE;

  return TRUE;
}


/*------------------------------------------------------------*/
void
vrml_def (char *def)
{
  /* pre */
  assert (indent_valid_state());
  assert (def);
  assert (vrml_valid_name (def));

  indent_check_buflen (4 + strlen (def));
  indent_blank();
  INDENT_FPRINTF (indent_file, "DEF %s", def);
  indent_needs_blank = TRUE;
}


/*------------------------------------------------------------*/
void
vrml_proto (char *proto)
{
  /* pre */
  assert (indent_valid_state());
  assert (proto);
  assert (vrml_valid_name (proto));

  indent_newline();
  INDENT_FPRINTF (indent_file, "PROTO %s", proto);
  indent_needs_blank = TRUE;	/* not strictly needed, but it looks nicer */
  vrml_begin_list();
}


/*------------------------------------------------------------*/
void
vrml_node (char *node)
{
  /* pre */
  assert (indent_valid_state());
  assert (node);
  assert (vrml_valid_name (node));

  indent_check_buflen (strlen (node) + 2);
  indent_blank();
  INDENT_FPRINTF (indent_file, "%s {", node);
  indent_increment_level (TRUE);
  indent_newline();
}


/*------------------------------------------------------------*/
void
vrml_list (char *list)
{
  /* pre */
  assert (indent_valid_state());
  assert (list);
  assert (vrml_valid_name (list));

  indent_check_buflen (strlen (list) + 2);
  indent_blank();
  INDENT_FPRINTF (indent_file, "%s [", list);
  indent_increment_level (TRUE);
  indent_newline();
}


/*------------------------------------------------------------*/
void
vrml_bbox (vector3 *center, vector3 *size)
{
  /* pre */
  assert (center);
  assert (size);

  if (size->x <= 0.0) return;
  if (size->y <= 0.0) return;
  if (size->z <= 0.0) return;

  indent_newline();
  indent_string ("bboxCenter");
  vrml_v3_g (center);
  indent_newline();
  indent_string ("bboxSize");
  vrml_v3_g (size);
}


/*------------------------------------------------------------*/
void
vrml_material (colour *dc, colour *ec, colour *sc,
	       double ai, double sh, double tr)
{
  boolean prev = FALSE;

  /* pre */
  assert (indent_valid_state());
  assert (ai >= 0.0);
  assert (ai <= 1.0);
  assert (sh >= 0.0);
  assert (sh <= 1.0);
  assert (tr >= 0.0);
  assert (tr <= 1.0);

  vrml_node ("Material");

  if (dc) {
    colour_copy_to_rgb (&rgb, dc);
    if (colour_unequal (&rgb, &default_diffusecolour)) {
      indent_check_buflen (18);
      INDENT_FPRINTF (indent_file,
		      "diffuseColor %.2g %.2g %.2g", rgb.x, rgb.y, rgb.z);
      prev = TRUE;
    }
  }

  if (ec) {
    colour_copy_to_rgb (&rgb, ec);
    if (colour_unequal (&rgb, &default_emissivecolour)) {
      if (prev) indent_newline();
      indent_check_buflen (19);
      INDENT_FPRINTF (indent_file,
		      "emissiveColor %.2g %.2g %.2g", rgb.x, rgb.y, rgb.z);
      prev = TRUE;
    }
  }

  if (sc) {
    colour_copy_to_rgb (&rgb, sc);
    if (colour_unequal (&rgb, &default_specularcolour)) {
      if (prev) indent_newline();
      indent_check_buflen (19);
      INDENT_FPRINTF (indent_file,
		      "specularColor %.2g %.2g %.2g", rgb.x, rgb.y, rgb.z);
      prev = TRUE;
    }
  }

  if (fabs (ai - 0.2) <= 0.0005) {
    if (prev) indent_newline();
    indent_check_buflen (18);
    INDENT_FPRINTF (indent_file, "ambientIntensity %.2g", ai);
    prev = TRUE;
  }

  if (fabs (sh - 0.2) <= 0.0005) {
    if (prev) indent_newline();
    indent_check_buflen (11);
    INDENT_FPRINTF (indent_file, "shininess %.2g", sh);
    prev = TRUE;
  }

  if (tr != 0.0) {
    if (prev) indent_newline();
    indent_check_buflen (14);
    INDENT_FPRINTF (indent_file, "transparency %.2g", tr);
  }

  vrml_finish_node();
}


/*------------------------------------------------------------*/
void
vrml_directionallight (double in, double ai, colour *c, vector3 *dir)
{
  boolean prev = FALSE;

  /* pre */
  assert (indent_valid_state());
  assert (in >= 0.0);
  assert (in <= 1.0);
  assert (ai >= 0.0);
  assert (ai <= 1.0);

  vrml_node ("DirectionalLight");

  if (in != 1.0) {
    indent_check_buflen (11);
    INDENT_FPRINTF (indent_file, "intensity %.2g", in);
    prev = TRUE;
  }

  if (ai != 0.0) {
    if (prev) indent_newline();
    indent_check_buflen (18);
    INDENT_FPRINTF (indent_file, "ambientIntensity %.2g", ai);
    prev = TRUE;
  }

  if (c) {
    colour_copy_to_rgb (&rgb, c);
    if (colour_unequal (&rgb, &white_colour)) {
      if (prev) indent_newline();
      indent_check_buflen (10);
      INDENT_FPRINTF (indent_file,
		      "color %.2g %.2g %.2g", rgb.x, rgb.y, rgb.z);
      prev = TRUE;
    }
  }

  if (dir) {
    if ((dir->x != 0.0) || (dir->y != 0.0) || (dir->z != -1.0)) {
      if (prev) indent_newline();
      indent_check_buflen (15);
      INDENT_FPRINTF (indent_file,
		      "direction %g %g %g", dir->x, dir->y, dir->z);
      prev = TRUE;
    }
  }

  vrml_finish_node();
}


/*------------------------------------------------------------*/
void
vrml_pointlight (double in, double ai, colour *c,
		 vector3 *loc, double r, vector3 *att)
{
  boolean prev = FALSE;

  /* pre */
  assert (indent_valid_state());
  assert (in >= 0.0);
  assert (in <= 1.0);
  assert (ai >= 0.0);
  assert (ai <= 1.0);
  assert (r >= 0.0);

  vrml_node ("PointLight");

  if (in != 1.0) {
    indent_check_buflen (11);
    INDENT_FPRINTF (indent_file, "intensity %.2g", in);
    prev = TRUE;
  }

  if (ai != 0.0) {
    if (prev) indent_newline();
    indent_check_buflen (18);
    INDENT_FPRINTF (indent_file, "ambientIntensity %.2g", ai);
    prev = TRUE;
  }

  if (c) {
    colour_copy_to_rgb (&rgb, c);
    if (colour_unequal (&rgb, &white_colour)) {
      if (prev) indent_newline();
      indent_check_buflen (10);
      INDENT_FPRINTF (indent_file,
		      "color %.2g %.2g %.2g", rgb.x, rgb.y, rgb.z);
      prev = TRUE;
    }
  }

  if (loc) {
    if ((loc->x != 0.0) || (loc->y != 0.0) || (loc->z != 0.0)) {
      if (prev) indent_newline();
      indent_check_buflen (14);
      INDENT_FPRINTF (indent_file,
		      "location %g %g %g", loc->x, loc->y, loc->z);
      prev = TRUE;
    }
  }

  if ((r != 100.0) && (r != 0.0)) {
    if (prev) indent_newline();
    indent_check_buflen (8);
    INDENT_FPRINTF (indent_file, "radius %g", r);
    prev = TRUE;
  }

  if (att) {
    if ((att->x != 1.0) || (att->y != 0.0) || (att->z != 0.0)) {
      indent_check_buflen (17);
      if (prev) indent_newline();
      INDENT_FPRINTF (indent_file,
		      "attenuation %g %g %g", att->x, att->y, att->z);
      prev = TRUE;
    }
  }

  vrml_finish_node();
}


/*------------------------------------------------------------*/
void
vrml_spotlight (double in, double ai, colour *c,
		vector3 *loc, vector3 *dir, double bw, double coa,
		double r, vector3 *att)
{
  boolean prev = FALSE;

  /* pre */
  assert (indent_valid_state());
  assert (in >= 0.0);
  assert (in <= 1.0);
  assert (ai >= 0.0);
  assert (ai <= 1.0);
  assert (bw >= 0.0);
  assert (bw < ANGLE_PI);
  assert (coa >= 0.0);
  assert (coa < ANGLE_PI);
  assert (r >= 0.0);

  vrml_node ("SpotLight");

  if (in != 1.0) {
    indent_check_buflen (11);
    INDENT_FPRINTF (indent_file, "intensity %.2g", in);
    prev = TRUE;
  }

  if (ai != 0.0) {
    if (prev) indent_newline();
    indent_check_buflen (18);
    INDENT_FPRINTF (indent_file, "ambientIntensity %.2g", ai);
    prev = TRUE;
  }

  if (c) {
    colour_copy_to_rgb (&rgb, c);
    if (colour_unequal (&rgb, &white_colour)) {
      if (prev) indent_newline();
      indent_check_buflen (10);
      INDENT_FPRINTF (indent_file,
		      "color %.2g %.2g %.2g", rgb.x, rgb.y, rgb.z);
      prev = TRUE;
    }
  }

  if (loc) {
    if ((loc->x != 0.0) || (loc->y != 0.0) || (loc->z != 0.0)) {
      if (prev) indent_newline();
      indent_check_buflen (14);
      INDENT_FPRINTF (indent_file,
		      "location %g %g %g", loc->x, loc->y, loc->z);
      prev = TRUE;
    }
  }

  if (dir) {
    if ((dir->x != 0.0) || (dir->y != 0.0) || (dir->z != -1.0)) {
      if (prev) indent_newline();
      indent_check_buflen (15);
      INDENT_FPRINTF (indent_file,
		      "direction %g %g %g", dir->x, dir->y, dir->z);
      prev = TRUE;
    }
  }

  if ((bw != ANGLE_PI / 2.0) || (bw != 0.0)) {
    if (prev) indent_newline();
    indent_check_buflen (11);
    INDENT_FPRINTF (indent_file, "beamWidth %g", bw);
    prev = TRUE;
  }

  if ((coa != ANGLE_PI / 4.0) || (coa != 0.0)) {
    if (prev) indent_newline();
    indent_check_buflen (13);
    INDENT_FPRINTF (indent_file, "cutOffAngle %g", coa);
    prev = TRUE;
  }

  if ((r != 100.0) || (r != 0.0)) {
    if (prev) indent_newline();
    indent_check_buflen (8);
    INDENT_FPRINTF (indent_file, "radius %g", r);
    prev = TRUE;
  }

  if (att) {
    if ((att->x != 1.0) || (att->y != 0.0) || (att->z != 0.0)) {
      if (prev) indent_newline();
      indent_check_buflen (17);
      INDENT_FPRINTF (indent_file,
		      "attenuation %g %g %g", att->x, att->y, att->z);
      prev = TRUE;
    }
  }

  vrml_finish_node();
}
