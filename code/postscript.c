/* postscript.c

   MolScript v2.1.2

   PostScript

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
     5-Oct-1997  began again
     7-Jan-1998  fixed assert bug in db_line
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "clib/str_utils.h"
#include "clib/angle.h"
#include "clib/matrix3.h"

#include "postscript.h"
#include "global.h"
#include "graphics.h"
#include "segment.h"
#include "state.h"


typedef struct {
  int code;
  int slot;
  double depth;
  state *st;
} depth_db_entry;

typedef struct {
  vector3 v1;
  vector3 v2;
  colour *c;
} line_db_entry;

typedef struct {
  vector3 v;
  colour *c;
} point_db_entry;

typedef struct {
  vector3 v;
  double rad;
  colour col;
  int donald_duck;
} sphere_db_entry;

typedef struct {
  vector3 p1, p2, p3, p4;
  colour col;
  double znorm;
} plane_db_entry;

typedef struct {
  vector3 pos;
  char *str;
  colour col;
} label_db_entry;

typedef struct {
  vector3 p1, p2;
  vector3 dir, perp;
  colour col;
  double taper;
} stick_db_entry;

enum db_codes { LINE_CODE, POINT_CODE, SPHERE_CODE,
		P_CODE, P1_CODE, P2_CODE, P3_CODE, P4_CODE, P12_CODE, P13_CODE,
		P14_CODE, P23_CODE, P34_CODE, P123_CODE, P134_CODE, P1234_CODE,
		T_CODE, T1_CODE, LABEL_CODE, STICK_CODE };


/*------------------------------------------------------------*/
static depth_db_entry *depth_array = NULL;
static int depth_alloc = 0;
static int depth_count = 0;

static line_db_entry *line_array = NULL;
static int line_alloc = 0;
static int line_count = 0;

static point_db_entry *point_array = NULL;
static int point_alloc = 0;
static int point_count = 0;

static sphere_db_entry *sphere_array = NULL;
static int sphere_alloc = 0;
static int sphere_count = 0;

static plane_db_entry *plane_array = NULL;
static int plane_alloc = 0;
static int plane_count = 0;

static label_db_entry *label_array = NULL;
static int label_alloc = 0;
static int label_count = 0;

static stick_db_entry *stick_array = NULL;
static int stick_alloc = 0;
static int stick_count = 0;


/*------------------------------------------------------------*/
static double bounding_box [4] = { -1.0, -1.0, -1.0, -1.0 };
static double default_area [4] = { 50.0, 100.0, 550.0, 700.0 };

static colour current_linecolour;
static colour current_spherecolour;
static double current_linewidth;
static double current_linedash;

static int line_output_count;
static int point_output_count;
static int sphere_output_count;
static int plane_output_count;
static int label_output_count;
static int stick_output_count;


/*------------------------------------------------------------*/
static void
output_colour (colour *c, int gsave)
{
  assert (c);
  assert (colour_valid_state (c));

  switch (c->spec) {
  case COLOUR_RGB:
    fprintf (outfile, "%.3g %.3g %.3g R", c->x, c->y, c->z);
    break;
  case COLOUR_HSB:
    fprintf (outfile, "%.3g %.3g %.3g H", c->x, c->y, c->z);
    break;
  case COLOUR_GREY:
    fprintf (outfile, "%.3g G", c->x);
    break;
  }
  if (gsave) fputc ('S', outfile);
}


/*------------------------------------------------------------*/
static void
output_linecolour (colour *c)
{
  assert (c);
  assert (colour_valid_state (c));

  if (colour_unequal (&current_linecolour, c)) {
    output_colour (c, FALSE);
    fputc ('\n', outfile);
    current_linecolour = *c;
  }
}


/*------------------------------------------------------------*/
static void
output_spherecolour (colour *c)
{
  assert (c);
  assert (colour_valid_state (c));

  if (colour_unequal (&current_spherecolour, c)) {
    PRINT ("/SC { ");
    output_colour (c, FALSE);
    PRINT (" } def\n");
    current_spherecolour = *c;
  }
}


/*------------------------------------------------------------*/
static void
output_linewidth (double lw)
{
  assert (lw >= 0.0);

  if (fabs (current_linewidth - lw) > 0.01) {
    fprintf (outfile, "%.3g LW\n", lw);
    current_linewidth = lw;
  }
}


/*------------------------------------------------------------*/
static void
output_linedash (double dash)
{
  assert (dash >= 0.0);

  if (fabs (current_linedash - dash) > 0.01) {
    if (dash >= 0.01) {
      fprintf (outfile, "[%.3g] D\n", dash);
    } else {
      fprintf (outfile, "ND\n");
    }
    current_linedash = dash;
  }
}


/*------------------------------------------------------------*/
static void
output_string (char *str, int len)
{
  int slot;

  assert (str);
  assert (*str);
  assert (len > 0);

  fputc ('(', outfile);

  for (slot = 0; slot < len; slot++) {
    switch (str[slot]) {
    case '%':			/* ignore percent; signifies blank to remove */
      break;
    case '(':			/* escape parenthesis and backslash */
    case ')':
    case '\\':
      fputc ('\\', outfile);	/* fall-through */
    default:
      fputc (str[slot], outfile);
      break;
    }
  }

  fputc (')', outfile);
}


/*------------------------------------------------------------*/
static void
db_init (void)
{
  int slot;
				/* deallocate previous data */
  for (slot = 0; slot < line_count; slot++) {
    if (line_array[slot].c) free (line_array[slot].c);
  }

  for (slot = 0; slot < label_count; slot++) {
    free (label_array[slot].str);
  }

  for (slot = 0; slot < point_count; slot++) {
    if (point_array[slot].c) free (point_array[slot].c);
  }

  if (depth_alloc == 0) {	/* allocate arrays */
    depth_alloc = 16384;
    depth_array = malloc (depth_alloc * sizeof (depth_db_entry));
  }
  depth_count = 0;

  if (line_alloc == 0) {
    line_alloc = 2048;
    line_array = malloc (line_alloc * sizeof (line_db_entry));
  }
  line_count = 0;

  if (point_alloc == 0) {
    point_alloc = 2048;
    point_array = malloc (point_alloc * sizeof (point_db_entry));
  }
  point_count = 0;

  if (sphere_alloc == 0) {
    sphere_alloc = 2048;
    sphere_array = malloc (sphere_alloc * sizeof (sphere_db_entry));
  }
  sphere_count = 0;

  if (plane_alloc == 0) {
    plane_alloc = 4096;
    plane_array = malloc (plane_alloc * sizeof (plane_db_entry));
  }
  plane_count = 0;

  if (label_alloc == 0) {
    label_alloc = 256;
    label_array = malloc (label_alloc * sizeof (label_db_entry));
  }
  label_count = 0;

  if (stick_alloc == 0) {
    stick_alloc = 256;
    stick_array = malloc (stick_alloc * sizeof (stick_db_entry));
  }
  stick_count = 0;
}


/*------------------------------------------------------------*/
static void
enter_depth (int code, int slot, double depth)
{
  depth_db_entry *de;

  assert (depth_array);

  if (depth_count >= depth_alloc) {
    depth_alloc *= 2;
    depth_array = realloc (depth_array, depth_alloc * sizeof (depth_db_entry));
  }

  de = depth_array + depth_count;
  de->code = code;
  de->slot = slot;
  de->depth = depth;
  de->st = current_state;

  depth_count++;
}


/*------------------------------------------------------------*/
static void
db_line (vector3 *v1, vector3 *v2, colour *c)
{
  line_db_entry *le;

  assert (line_array);
  assert (v1);
  assert (v2);

  if (v3_distance (v1, v2) == 0.0) return;
  if (outside_extent_2v (v1, v2)) return;

  if (v3_distance (v1, v2) > current_state->segmentsize) {
    vector3 middle;

    v3_middle (&middle, v1, v2);

    db_line (v1, &middle, c);
    db_line (&middle, v2, c);

  } else {

    if (line_count >= line_alloc) {
      line_alloc *= 2;
      line_array = realloc (line_array, line_alloc * sizeof (line_db_entry));
    }

    le = line_array + line_count;
    le->v1 = *v1;
    le->v2 = *v2;
    if (c) {
      le->c = colour_clone (c);
    } else {
      le->c = NULL;
    }

    enter_depth (LINE_CODE, line_count++, (v1->z + v2->z) / 2.0);
  }
}


/*------------------------------------------------------------*/
static void
db_point (vector3 *v, colour *c)
{
  point_db_entry *pte;

  assert (point_array);
  assert (v);

  if (outside_extent_radius (v, 0.0)) return;

  if (point_count >= point_alloc) {
    point_alloc *= 2;
    point_array = realloc (point_array, point_alloc * sizeof (point_db_entry));
  }

  pte = point_array + point_count;
  pte->v = *v;
  if (c) {
    pte->c = colour_clone (c);
  } else {
    pte->c = NULL;
  }

  enter_depth (POINT_CODE, point_count++, v->z);
}


/*------------------------------------------------------------*/
static void
db_plane (vector3 *p1, vector3 *p2, vector3 *p3, vector3 *p4,
	  colour *c, double znorm, int code)
{
  plane_db_entry *pe;
  double depth;

  assert (plane_array);
  assert (p1);
  assert (p2);
  assert (p3);
  assert (c);
  assert (znorm >= -1.0);
  assert (znorm <= 1.0);

  if (znorm < 0.0) return;	/* remove planes facing away */

  if (plane_count >= plane_alloc) {
    plane_alloc *= 2;
    plane_array = realloc (plane_array, plane_alloc * sizeof (plane_db_entry));
  }

  pe = plane_array + plane_count;
  pe->col = *c;
  pe->znorm = znorm;
  pe->p1 = *p1;
  pe->p2 = *p2;

  if ((code == T_CODE) || (code == T1_CODE)) {
    pe->p3 = *p3;
    v3_initialize (&(pe->p4), 0.0, 0.0, 0.0);

  } else {
    vector3 vec;
    double len1, len2;

    v3_difference (&vec, p1, p4);
    vec.z = 0.0;
    len1 = v3_length (&vec);
    v3_difference (&vec, p2, p3);
    vec.z = 0.0;
    len1 += v3_length (&vec);

    v3_difference (&vec, p1, p3);
    vec.z = 0.0;
    len2 = v3_length (&vec);
    v3_difference (&vec, p2, p4);
    vec.z = 0.0;
    len2 += v3_length (&vec);

    if (len1 < len2) {		/* plane is not twisted in the xy-plane */
      pe->p3 = *p3;
      pe->p4 = *p4;
    } else {			/* plane is twisted; correct it */
      pe->p3 = *p4;
      pe->p4 = *p3;
    }
  }

  depth = (p1->z > p2->z) ? p1->z : p2->z;
  if (p3->z > depth) depth = p3->z;
  if (p4 && (p4->z > depth)) depth = p4->z;

  enter_depth (code, plane_count++, depth);
}


/*------------------------------------------------------------*/
static void
db_plane_add_edge (int slot, int first)
{
  depth_db_entry *de;

  assert (slot >= 0);
  assert (slot < depth_count);

  de = depth_array + slot;

  switch (de->code) {

  case P_CODE:
    de->code = first ? P4_CODE : P2_CODE;
    break;

  case P1_CODE:
    de->code = first ? P14_CODE : P12_CODE;
    break;

  case P3_CODE:
    de->code = first ? P34_CODE : P23_CODE;
    break;

  case P13_CODE:
    de->code = first ? P134_CODE : P123_CODE;
    break;

  case P134_CODE:
  case P123_CODE:
  case P1234_CODE:
    break;

    /*
  default:
    assert (FALSE);
    */
  }
}


/*------------------------------------------------------------*/
static void
db_tri (vector3 *p1, vector3 *p2, vector3 *p3,
	colour *c, double znorm, int code)
{
  assert (p1);
  assert (p2);
  assert (p3);
  assert (c);

  if (znorm < 0.0) return;

  if ((v3_distance (p1, p2) > current_state->segmentsize) ||
      (v3_distance (p2, p3) > current_state->segmentsize) ||
      (v3_distance (p3, p1) > current_state->segmentsize)) {

    vector3 p12, p23, p31;

    v3_middle (&p12, p1, p2);
    v3_middle (&p23, p2, p3);
    v3_middle (&p31, p3, p1);

    db_tri (p1, &p12, &p31, c, znorm, code);
    db_tri (&p12, p2, &p23, c, znorm, code);
    db_tri (&p12, &p23, &p31, c, znorm, T_CODE);
    db_tri (&p31, &p23, p3, c, znorm, T_CODE);

  } else {
    db_plane (p1, p2, p3, NULL, c, znorm, code);
  }
}


/*------------------------------------------------------------*/
static void
db_sphere (vector3 *v, double r, colour *c, int donald_duck)
{
  sphere_db_entry *se;

  assert (sphere_array);
  assert (v);
  assert (r > 0.0);
  assert (c);

  if (outside_extent_radius (v, r)) return;

  if (sphere_count >= sphere_alloc) {
    sphere_alloc *= 2;
    sphere_array = realloc (sphere_array, sphere_alloc * sizeof (sphere_db_entry));
  }

  se = sphere_array + sphere_count;
  se->v = *v;
  se->rad = r;
  se->col = *c;
  se->donald_duck = donald_duck;

  enter_depth (SPHERE_CODE, sphere_count++, se->v.z + 0.5 * se->rad);
}


/*------------------------------------------------------------*/
static int
depth_compare (const void *de1, const void *de2)
{
  if (((depth_db_entry *) de1)->depth < ((depth_db_entry *) de2)->depth)
    return -1;
  else if (((depth_db_entry *) de1)->depth > ((depth_db_entry *) de2)->depth)
    return 1;
  else
    return 0;
}


/*------------------------------------------------------------*/
static void
db_depth_sort (void)
{
  if (depth_count > 0)
    qsort (depth_array, depth_count, sizeof (depth_db_entry), depth_compare);
}


/*------------------------------------------------------------*/
static void
db_transform (double matrix[4][4], double scale)
{
  int slot;
  line_db_entry *le;
  point_db_entry *pte;
  sphere_db_entry *se;
  plane_db_entry *pe;
  label_db_entry *lae;
  stick_db_entry *ste;

  le = line_array;
  for (slot = 0; slot < line_count; slot++, le++) {
    matrix3_transform (&(le->v1), matrix);
    matrix3_transform (&(le->v2), matrix);
  }

  pte = point_array;
  for (slot = 0; slot < point_count; slot++, pte++) {
    matrix3_transform (&(pte->v), matrix);
  }

  se = sphere_array;
  for (slot = 0; slot < sphere_count; slot++, se++) {
    matrix3_transform (&(se->v), matrix);
    se->rad *= scale;
  }

  pe = plane_array;
  for (slot = 0; slot < plane_count; slot++, pe++) {
    matrix3_transform (&(pe->p1), matrix);
    matrix3_transform (&(pe->p2), matrix);
    matrix3_transform (&(pe->p3), matrix);
    matrix3_transform (&(pe->p4), matrix);
  }

  lae = label_array;
  for (slot = 0; slot < label_count; slot++, lae++) {
    matrix3_transform (&(lae->pos), matrix);
  }

  ste = stick_array;
  for (slot = 0; slot < stick_count; slot++, ste++) {
    matrix3_transform (&(ste->p1), matrix);
    matrix3_transform (&(ste->p2), matrix);
    v3_scale (&(ste->perp), scale);
  }
}


/*------------------------------------------------------------*/
static double
shade_factor (double znorm, depth_db_entry *de)
{
  assert (znorm >= 0.0);
  assert (znorm <= 1.0);
  assert (de);

  return (de->st->shading * pow (znorm, de->st->shadingexponent) +
	  1.0 - de->st->shading);
}


/*------------------------------------------------------------*/
static void
db_plane_output (depth_db_entry *de, char *plane_type)
{
  plane_db_entry *pe;

  assert (de);
  assert (plane_type);
  assert (*plane_type);

  pe = plane_array + de->slot;
  colour_darker (&(pe->col), shade_factor (pe->znorm, de), &(pe->col));

  if (! str_eq ("P", plane_type)) {
    output_linecolour (&(de->st->linecolour));
    output_linewidth (depthcue (de->depth, de->st) * de->st->linewidth);
    output_linedash (de->st->linedash);
  }
  output_colour (&(pe->col), TRUE);
  fprintf (outfile, " %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %s\n",
	   pe->p4.x, pe->p4.y, pe->p3.x, pe->p3.y,
	   pe->p2.x, pe->p2.y, pe->p1.x, pe->p1.y, plane_type);

  plane_output_count++;
}


/*------------------------------------------------------------*/
static void
db_tri_output (depth_db_entry *de, char *tri_type)
{
  plane_db_entry *pe;
  double factor;

  assert (de);
  assert (tri_type);
  assert (*tri_type);

  pe = plane_array + de->slot;
  factor = de->st->shading * pow (pe->znorm, de->st->shadingexponent) +
           1.0 - de->st->shading;
  colour_darker (&(pe->col), factor, &(pe->col));

  if (! str_eq ("T", tri_type)) {
    output_linecolour (&(de->st->linecolour));
    output_linewidth (depthcue (de->depth, de->st) * de->st->linewidth);
    output_linedash (de->st->linedash);
  }
  output_colour (&(pe->col), TRUE);
  fprintf (outfile, " %.2f %.2f %.2f %.2f %.2f %.2f %s\n",
	   pe->p3.x, pe->p3.y, pe->p2.x, pe->p2.y,
	   pe->p1.x, pe->p1.y, tri_type);

  plane_output_count++;
}


/*------------------------------------------------------------*/
static void
db_string_output (char *str, int length, int greek, depth_db_entry *de)
{
  assert (str);
  assert (*str);
  assert (length > 0);
  assert (de);

  output_string (str, length);
  if (greek) {
    PRINT (" FG");
  } else {
    PRINT (" FR");
  }
  fprintf (outfile, " %.2f",
	            depthcue (de->depth, de->st) * de->st->labelsize);
  if (de->st->labelbackground != 0.0)
    fprintf (outfile, " %.2f PRB", de->st->labelbackground);
  fprintf (outfile, " PR\n");
}


/*------------------------------------------------------------*/
static void
db_label_output (depth_db_entry *de)
{
  label_db_entry *lae;
  int len, slot, prev_greek, greek, first;
  int *labelmask;

  assert (de);

  lae = label_array + de->slot;
  len = strlen (lae->str);

  output_linecolour (&(lae->col));

  fprintf (outfile, "%.2f %.2f M", lae->pos.x, lae->pos.y);
  if (de->st->labelcentre) {
    fputc (' ', outfile);
    output_string (lae->str, len);
    fprintf (outfile, " %.2f FR",
	              depthcue (de->depth, de->st) * de->st->labelsize);
    if (de->st->labelrotation) {
      fprintf (outfile, " CR90\n");
    } else {
      fprintf (outfile, " C\n");
    }
  } else {
    fputc ('\n', outfile);
  }

  if (de->st->labelrotation) fprintf (outfile, "R90\n");

  if (de->st->labelmasklength > 0) {
    labelmask = calloc (len, sizeof (int));
    memcpy (labelmask, de->st->labelmask, de->st->labelmasklength);

    first = 0;
    prev_greek = (de->st->labelmask[0] / 3) == 1;
    for (slot = 1; slot < len; slot++) {
      greek = (de->st->labelmask[slot] / 3) == 1;
      if (greek != prev_greek) {
	db_string_output (lae->str + first, slot - first, prev_greek, de);
	first = slot;
	prev_greek = greek;
      }
    }
    db_string_output (lae->str + first, len - first, prev_greek, de);

    free (labelmask);

  } else {
    db_string_output (lae->str, len, FALSE, de);
  }

  if (de->st->labelrotation) fprintf (outfile, "grestore\n");

  label_output_count++;
}


/*------------------------------------------------------------*/
static void
db_output (void)
{
  int slot;
  double rval;
  depth_db_entry *de;
  line_db_entry *le;
  point_db_entry *pte;
  sphere_db_entry *se;
  stick_db_entry *ste;

  line_output_count = 0;
  point_output_count = 0;
  sphere_output_count = 0;
  plane_output_count = 0;
  label_output_count = 0;
  stick_output_count = 0;

  for (slot = 0; slot < depth_count; slot++) {
    de = depth_array + slot;

    switch (de->code) {

    case LINE_CODE:
      le = line_array + de->slot;
      if (le->c) {
	output_linecolour (le->c);
      } else {
	output_linecolour (&(de->st->linecolour));
      }
      output_linewidth (depthcue (de->depth, de->st) * de->st->linewidth);
      output_linedash (de->st->linedash);
      fprintf (outfile, "%.2f %.2f %.2f %.2f L\n",
		       le->v1.x, le->v1.y, le->v2.x, le->v2.y);
      line_output_count++;
      break;

    case POINT_CODE:
      pte = point_array + de->slot;
      if (pte->c) {
	output_linecolour (pte->c);
      } else {
	output_linecolour (&(de->st->linecolour));
      }
      output_linewidth (depthcue (de->depth, de->st) * de->st->linewidth);
      output_linedash (0.0);
      fprintf (outfile, "%.2f %.2f PT\n", pte->v.x, pte->v.y);
      point_output_count++;
      break;

    case SPHERE_CODE:
      se = sphere_array + de->slot;
      output_linecolour (&(de->st->linecolour));
      output_linewidth (depthcue (de->depth, de->st) * de->st->linewidth);
      output_linedash (de->st->linedash);
      output_spherecolour (&(se->col));
      fprintf (outfile, "%.2f %.2f %.2f %s\n",
	       se->v.x, se->v.y, se->rad, se->donald_duck ? "SD" : "SS");
      sphere_output_count++;
      break;

    case P_CODE:
      db_plane_output (de, "P");
      break;

    case P1_CODE:
      db_plane_output (de, "P1");
      break;
 
    case P2_CODE:
      db_plane_output (de, "P2");
      break;
 
    case P3_CODE:
      db_plane_output (de, "P3");
      break;
 
    case P4_CODE:
      db_plane_output (de, "P4");
      break;
 
    case P13_CODE:
      db_plane_output (de, "P13");
      break;

    case P12_CODE:
      db_plane_output (de, "P12");
      break;

    case P14_CODE:
      db_plane_output (de, "P14");
      break;

    case P23_CODE:

      db_plane_output (de, "P23");
      break;

    case P34_CODE:
      db_plane_output (de, "P34");
      break;

    case P123_CODE:
      db_plane_output (de, "P123");
      break;

    case P134_CODE:
      db_plane_output (de, "P134");
      break;

    case P1234_CODE:
      db_plane_output (de, "P1234");
      break;

    case T_CODE:
      db_tri_output (de, "T");
      break;

    case T1_CODE:
      db_tri_output (de, "T1");
      break;

    case LABEL_CODE:
      if (de->st->labelclip) db_label_output (de); /* clipped labels */
      break;

    case STICK_CODE:
      ste = stick_array + de->slot;
      output_linecolour (&(de->st->linecolour));
      output_linewidth (depthcue (de->depth, de->st) * de->st->linewidth);
      output_linedash (de->st->linedash);
				/* elliptical arc part of stick */
      fprintf (outfile, "%.2f %.2f %.2f",
	       ste->p2.x, ste->p2.y, v3_length (&(ste->perp)) * ste->taper);
      rval = ste->dir.x / sqrt (ste->dir.x * ste->dir.x +
				ste->dir.y * ste->dir.y);
      rval = (ste->dir.y >= 0.0) ?
	     to_degrees (acos (rval)) : (360.0 - to_degrees (acos (rval)));
      fprintf (outfile, " %.2f %.2f", rval, fabs (ste->dir.z));
				/* straight line part of stick */
      fprintf (outfile, " %.2f %.2f %.2f %.2f SP ",
	                ste->p1.x - ste->perp.x, ste->p1.y - ste->perp.y,
	                ste->p1.x + ste->perp.x, ste->p1.y + ste->perp.y);
      output_colour (&(ste->col), FALSE);
      fprintf (outfile, " SF\n");
      stick_output_count++;
      break;
    }
  }

  for (slot = 0; slot < depth_count; slot++) {	/* unclipped labels */
    de = depth_array + slot;
    if ((de->code == LABEL_CODE) &&
	(! de->st->labelclip)) db_label_output (de);
  }
}


/*------------------------------------------------------------*/
void
ps_set (void)
{
  output_first_plot = ps_first_plot;
  output_finish_output = ps_finish_output;
  output_start_plot = ps_start_plot;
  output_finish_plot = ps_finish_plot;

  set_area = ps_set_area;
  set_background = ps_set_background;
  anchor_start = do_nothing_str;
  anchor_description = do_nothing_str;
  anchor_parameter = do_nothing_str;
  anchor_start_geometry = do_nothing;
  anchor_finish = do_nothing;
  lod_start = do_nothing;
  lod_finish = do_nothing;
  lod_start_group = do_nothing;
  lod_finish_group = do_nothing;
  viewpoint_start = do_nothing_str;
  viewpoint_output = do_nothing;
  output_directionallight = do_nothing;
  output_pointlight = do_nothing;
  output_spotlight = do_nothing;
  output_comment = do_nothing_str;

  output_coil = ps_coil;
  output_cylinder = ps_cylinder;
  output_helix = ps_helix;
  output_label = ps_label;
  output_line = ps_line;
  output_sphere = ps_sphere;
  output_stick = ps_stick;
  output_strand = ps_strand;

  output_start_object = do_nothing;
  output_object = ps_object;
  output_finish_object = do_nothing;

  output_pickable = NULL;

  output_mode = POSTSCRIPT_MODE;
}


/*------------------------------------------------------------*/
void
ps_first_plot (void)
{
  if (!first_plot) return;

  set_outfile ("w");

  if (fprintf (outfile, "%%!PS-Adobe-3.0\n") < 0)
    yyerror ("could not write to the output PostScript file");

  PRINT ("%%BoundingBox: (atend)\n");
  if (title) fprintf (outfile, "%%%%Title: %s\n", title);
  fprintf (outfile, "%%%%Creator: %s, %s\n", program_str, copyright_str);
  if (user_str[0] != '\0') fprintf (outfile, "%%%%For: %s\n", user_str);
  PRINT ("%%DocumentNeededResources: font Times-Roman Symbol\n");
  PRINT ("%%Pages: 1\n");
  PRINT ("%%EndComments\n");
  PRINT ("%%BeginProlog\n");
  PRINT ("50 dict begin\n");
  PRINT ("/R { setrgbcolor } bind def\n");
  PRINT ("/G { setgray } bind def\n");
  PRINT ("/H { sethsbcolor } bind def\n");
  PRINT ("/LW { setlinewidth } bind def\n");
  PRINT ("/D { 0 setdash 0 setlinecap } bind def\n");
  PRINT ("/ND { [] 0 setdash 1 setlinecap } bind def\n");
  PRINT ("/RS { gsave setrgbcolor } bind def\n");
  PRINT ("/HS { gsave sethsbcolor } bind def\n");
  PRINT ("/GS { gsave setgray } bind def\n");
  PRINT ("/M { moveto } bind def\n");
  PRINT ("/L { moveto lineto stroke } bind def\n");
  PRINT ("/PT { 2 copy moveto lineto stroke } bind def\n");
  PRINT ("/SC { 1 G } def\n");
  PRINT ("/SS { newpath 0 360 arc gsave SC fill grestore stroke } bind def\n");
  PRINT ("/SD {\n");
  PRINT ("  3 copy newpath 0 360 arc gsave SC fill 0 setgray 0.5 setlinewidth\n");
  PRINT ("  3 copy 0.94 mul 260 350 arc stroke 3 copy 0.87 mul 275 335 arc stroke\n");
  PRINT ("  3 copy 0.79 mul 295 315 arc stroke 3 copy 0.8 mul 115 135 arc\n");
  PRINT ("  3 copy 0.6 mul 135 115 arcn closepath gsave 1 setgray fill grestore stroke\n");
  PRINT ("  3 copy 0.7 mul 115 135 arc stroke 3 copy 0.6 mul 124.9 125 arc\n");
  PRINT ("  0.8 mul 125 125.1 arc stroke grestore stroke\n");
  PRINT (" } bind def\n");
  PRINT ("/T { moveto lineto lineto fill grestore } bind def\n");
  PRINT ("/T1 { 4 copy 10 4 roll T moveto lineto stroke } bind def\n");
  PRINT ("/P { moveto lineto lineto lineto fill grestore } bind def\n");
  PRINT ("/P1 { 4 copy 12 4 roll P moveto lineto stroke } bind def\n");
  PRINT ("/P2 { 8 2 roll P1 } bind def\n");
  PRINT ("/P3 { 8 4 roll P1 } bind def\n");
  PRINT ("/P4 { 8 -2 roll P1 } bind def\n");
  PRINT ("/P12 { 8 copy P moveto lineto lineto stroke pop pop } bind def\n");
  PRINT ("/P13 { 8 copy P moveto lineto moveto lineto stroke } bind def\n");
  PRINT ("/P14 { 8 copy P 8 -2 roll moveto lineto lineto stroke pop pop } bind def\n");
  PRINT ("/P23 { 8 2 roll P12 } bind def\n");
  PRINT ("/P34 { 8 4 roll P12 } bind def\n");
  PRINT ("/P123 { 8 copy P moveto lineto lineto lineto stroke } bind def\n");
  PRINT ("/P134 { 8 copy P 4 2 roll moveto lineto 4 2 roll lineto lineto stroke } bind def\n");
  PRINT ("/P1234 { 8 copy P moveto lineto lineto lineto closepath stroke } bind def\n");
  PRINT ("/FR { /Times-Roman findfont } bind def\n");
  PRINT ("/FG { /Symbol findfont } bind def\n");
  PRINT ("/PR { scalefont setfont show } bind def\n");
  PRINT ("/C {\n");
  PRINT ("  1 index scalefont setfont\n");
  PRINT ("  exch stringwidth pop -2 div exch -3 div rmoveto\n");
  PRINT (" } bind def\n");
  PRINT ("/CR90 {\n");
  PRINT ("  1 index scalefont setfont\n");
  PRINT ("  exch stringwidth pop -2 div exch 3 div exch rmoveto\n");
  PRINT (" } bind def\n");
  PRINT ("/R90 { gsave currentpoint translate 90 rotate } bind def\n");
  PRINT ("/PRB { gsave setlinewidth 3 copy scalefont setfont\n");
  PRINT ("  false charpath BC stroke grestore } bind def\n");
  PRINT ("/EA {\n");
  PRINT ("  matrix currentmatrix 8 1 roll 7 -2 roll translate\n");
  PRINT ("  4 -1 roll rotate 3 -1 roll 1 scale 0 0 5 2 roll arc setmatrix\n");
  PRINT (" } bind def\n");
  PRINT ("/SP { moveto lineto -90 90 EA closepath gsave } bind def\n");
  PRINT ("/SF { fill grestore stroke } bind def\n");
  PRINT ("%%EndProlog\n");
  PRINT ("%%BeginSetup\n");
  PRINT ("%%IncludeResource: font Times-Roman\n");
  PRINT ("%%IncludeResource: font Symbol\n");
  PRINT ("1 setlinejoin 1 LW 0 G ND newpath\n");
  PRINT ("%%EndSetup\n");
  PRINT ("%%Page: 1 1\n");
}


/*------------------------------------------------------------*/
void
ps_finish_output (void)
{
  int i0, i1, i2, i3;

  PRINT ("end\n");
  PRINT ("showpage\n");
  PRINT ("%%Trailer\n");
  if (bounding_box[0] < 0.0) {
    i0 = floor (default_area[0]);
    i1 = floor (default_area[1]);
    i2 = ceil (default_area[2]);
    i3 = ceil (default_area[3]);
  } else {
    i0 = floor (bounding_box[0]);
    i1 = floor (bounding_box[1]);
    i2 = ceil (bounding_box[2]);
    i3 = ceil (bounding_box[3]);
  } 
  fprintf (outfile, "%%%%BoundingBox: %i %i %i %i\n", i0, i1, i2, i3);
  PRINT ("%%EOF\n");
}


/*------------------------------------------------------------*/
void
ps_start_plot (void)
{
  set_area_values (default_area[0], default_area[1],
		   default_area[2], default_area[3]);

  db_init();

  current_linecolour = black_colour;
  current_spherecolour = white_colour;
  current_linewidth = 1.0;
  current_linedash = 0.0;
  background_colour = white_colour;

  PRINT ("/MolScriptPlotSave save def\n");
}


/*------------------------------------------------------------*/
void
ps_finish_plot (void)
{
  double matrix[4][4];
  double temp_matrix[4][4];
  double scale;

  if (bounding_box[0] < 0.0) {
    bounding_box[0] = area[0];
    bounding_box[1] = area[1];
    bounding_box[2] = area[2];
    bounding_box[3] = area[3];
  } else {
    if (area[0] < bounding_box[0]) bounding_box[0] = area[0];
    if (area[1] < bounding_box[1]) bounding_box[1] = area[1];
    if (area[2] > bounding_box[2]) bounding_box[2] = area[2];
    if (area[3] > bounding_box[3]) bounding_box[3] = area[3];
  }

  fprintf (outfile, "%.3g %.3g moveto %.3g %.3g lineto %.3g %.3g lineto %.3g %.3g lineto closepath gsave\n",
	   area[0], area[1], area[2], area[1], area[2], area[3], area[0], area[3]);

  PRINT ("/BC { ");
  output_colour (&background_colour, FALSE);
  PRINT (" } def\n");
  PRINT ("gsave BC fill grestore\nclip newpath\n");

  set_extent();

  matrix3_orthographic_projection (matrix,
				   -window, window,
				   -window, window,
				   -1.0, 1.0);
  if (area[2] - area[0] < area[3] - area[1]) {
    scale = (area[2] - area[0]) / 2.0;
  } else {
    scale = (area[3] - area[1]) / 2.0;
  }
  matrix3_scale (temp_matrix, scale, scale, 1.0);
  matrix3_concatenate (matrix, temp_matrix);
  matrix3_translation (temp_matrix, (area[0] + area[2]) / 2.0,
		                    (area[1] + area[3]) / 2.0, 0.0);
  matrix3_concatenate (matrix, temp_matrix);

  if (aspect_ratio > 1.0) {	/* scale factor from PostScript to Angstrom */
    scale = ((area[2] - area[0]) / (2.0 * window));
  } else {
    scale = ((area[3] - area[1]) / (2.0 * window));
  }

  db_depth_sort();
  db_transform (matrix, scale);
  db_output();

  PRINT ("grestore ");
  if (frame) {
    PRINT ("stroke");
  } else {
    PRINT ("newpath");
  }
  PRINT ("\nMolScriptPlotSave restore\n");

  if (message_mode)
    fprintf (stderr,
	     "%i lines, %i points, %i spheres, %i planes, %i sticks and %i labels.\n",
	     line_output_count, point_output_count, sphere_output_count,
	     plane_output_count, stick_output_count, label_output_count);
  /*
	     line_count, point_count, sphere_count, plane_count,
	     stick_count, label_count);
	     */
}


/*------------------------------------------------------------*/
void
ps_set_area (void)
{
  assert (dstack_size == 4);

  set_area_values (dstack[0], dstack[1], dstack[2], dstack[3]);
  clear_dstack ();

  if ((area[2] < area[0]) || (area[3] < area[1]))
    yyerror ("invalid area values");
}


/*------------------------------------------------------------*/
void
ps_set_background (void)
{
  background_colour = given_colour;
}


/*------------------------------------------------------------*/
void
ps_coil (void)
{
  int slot, code;
  coil_segment *cs;
  vector3 dir, perp, prev_perp, p1, p2, p3, p4;
  double radius;
  colour *col;
  colour shade;

  cs = coil_segments;
  v3_difference (&dir, &((cs+1)->p), &(cs->p));
  v3_normalize (&dir);
  v3_cross_product (&perp, &dir, &zaxis);
  v3_normalize (&perp);
  v3_scaled (&dir,
	     current_state->coilradius * depthcue (cs->p.z, current_state),
	     &perp);
  v3_sum (&p2, &(cs->p), &dir);
  v3_difference (&p3, &(cs->p), &dir);
  if (! current_state->colourparts) col = &(current_state->planecolour);

  for (slot = 1; slot < coil_segment_count; slot++) {
    cs = coil_segments + slot;

    prev_perp = perp;
    p1 = p2;
    p4 = p3;

    if (slot == 1) {
      v3_difference (&dir, &((cs+1)->p), &((cs-1)->p));
      code = P134_CODE;
    } else if (slot == coil_segment_count - 1) {
      v3_difference (&dir, &(cs->p), &((cs-1)->p));
      code = P123_CODE;
    } else {
      v3_difference (&dir, &((cs+1)->p), &((cs-1)->p));
      code = P13_CODE;
    }
    v3_normalize (&dir);
    v3_cross_product (&perp, &dir, &zaxis);
    v3_normalize (&perp);
    radius = current_state->coilradius * depthcue (cs->p.z, current_state);
    v3_sum_scaled (&p2, &(cs->p), radius, &perp);
    v3_sum_scaled (&p3, &(cs->p), -radius, &perp);
    if (current_state->colourparts) col = &(cs->c);
    db_plane (&p1, &p2, &p3, &p4, col, sin (acos (dir.z)), code);

				/* coil changes dir with respect to viewer */
    if (v3_dot_product (&perp, &prev_perp) <= 0.5) {
      colour_darker (&shade,
		     shade_factor (sin (acos (dir.z)),
				   depth_array + depth_count-1),
		     &(current_state->planecolour));
      db_sphere (&(cs->p),
		 current_state->coilradius * depthcue (cs->p.z, current_state),
		 &shade, FALSE);
    }
  }
}


/*------------------------------------------------------------*/
static void
cylinder_slice (vector3 *v1, vector3 *v2, int code,
		int segments, vector3 *arad, vector3 *brad)
{
  int slot;
  vector3 axis, rad, prev_rad, p1, p2, p3, p4;
  double angle, znorm, prev_znorm;

  assert (v1);
  assert (v2);
  assert (v3_distance (v1, v2) > 0.0);
  assert (segments > 4);
  assert (arad);
  assert (brad);

  v3_difference (&axis, v2, v1);

  angle = 2.0 * ANGLE_PI * -1.0 / (double) segments;
  v3_scaled (&prev_rad, cos (angle), arad);
  v3_add_scaled (&prev_rad, sin (angle), brad);
  v3_sum_scaled (&p2, v1, current_state->cylinderradius, arad);
  v3_sum (&p3, &p2, &axis);
  rad = *arad;
  znorm = 0.5 * (prev_rad.z + rad.z);

  for (slot = 1; slot <= segments; slot++) {
    p1 = p2;
    p4 = p3;
    prev_rad = rad;
    prev_znorm = znorm;

    angle = 2.0 * ANGLE_PI * (double) slot / (double) segments;
    v3_scaled (&rad, cos (angle), arad);
    v3_add_scaled (&rad, sin (angle), brad);
    v3_sum_scaled (&p2, v1, current_state->cylinderradius, &rad);
    v3_sum (&p3, &p2, &axis);
    znorm = 0.5 * (prev_rad.z + rad.z);
    db_plane (&p1, &p2, &p3, &p4,
	      &(current_state->planecolour), znorm, code);
    if (prev_znorm * znorm < 0.0) {
      if (znorm >= 0.0) {
	db_plane_add_edge (depth_count - 1, TRUE);
      } else {
	db_plane_add_edge (depth_count - 1, FALSE);
      }
    }
  }
}

/*------------------------------------------------------------*/
void
ps_cylinder (vector3 *v1, vector3 *v2)
{
  int slot, slices;
  vector3 axis, arad, brad, rad, center, p1, p2;
  double angle, znorm, len;
  int segments = 3 * current_state->segments + 5;

  assert (v1);
  assert (v2);
  assert (v3_distance (v1, v2) > 0.0);

  v3_difference (&axis, v2, v1);
  v3_normalize (&axis);
  v3_cross_product (&arad, &axis, &xaxis);
  if (v3_length (&arad) < 1.0e-10) v3_cross_product (&arad, &axis, &yaxis);
  v3_cross_product (&brad, &arad, &axis);
  v3_normalize (&brad);
  v3_cross_product (&arad, &brad, &axis);
  v3_normalize (&arad);

  if (axis.z > 0.0) center = *v2; else center = *v1;
  znorm = fabs (axis.z);

  v3_sum_scaled (&p2, &center, current_state->cylinderradius, &arad);
  for (slot = 1; slot < segments; slot++) {
    p1 = p2;
    angle = 2.0 * ANGLE_PI * (double) slot / (double) segments;
    v3_scaled (&rad, cos (angle), &arad);
    v3_add_scaled (&rad, sin (angle), &brad);
    v3_sum_scaled (&p2, &center, current_state->cylinderradius, &rad);
    db_tri (&p1, &p2, &center, &(current_state->planecolour), znorm, T1_CODE);
  }
  v3_sum_scaled (&p1, &center, current_state->cylinderradius, &arad);
  db_tri (&p1, &p2, &center, &(current_state->planecolour), znorm, T1_CODE);

  len = v3_distance (v1, v2);
  slices = 1 + (int) (len / current_state->segmentsize);
  if (slices < 2) slices = 2;

  v3_sum_scaled (&p1, v1, len / (double) slices, &axis);
  cylinder_slice (v1, &p1, P1_CODE, segments, &arad, &brad);

  for (slot = 1; slot < slices - 1; slot++) {
    v3_sum_scaled (&p1, v1, len * (double) slot / (double) slices, &axis);
    v3_sum_scaled (&p2, v1, len * (double) (slot+1) / (double) slices, &axis);
    cylinder_slice (&p1, &p2, P_CODE, segments, &arad, &brad);
  }

  v3_sum_scaled (&p1, v1, len * (double) (slices-1) / (double) slices, &axis);
  cylinder_slice (&p1, v2, P3_CODE, segments, &arad, &brad);
}


/*------------------------------------------------------------*/
void
ps_helix (void)
{
  int slot, first;
  double znorm, prev_znorm;
  vector3 normal, pos, dir1, dir2;
  helix_segment *hs, *nhs;

  first = depth_count;

  if (current_state->colourparts) {

    for (slot = 0; slot < helix_segment_count - 1; slot++) {
      hs = helix_segments + slot;
      nhs = hs + 1;
      v3_middle (&pos, &(hs->p1), &(hs->p2));
      v3_difference (&dir1, &(nhs->p1), &pos);
      v3_difference (&dir2, &(nhs->p2), &pos);
      v3_cross_product (&normal, &dir2, &dir1);
      v3_normalize (&normal);
      znorm = normal.z / sin (acos (nhs->a.z));
      db_plane (&(hs->p1), &(nhs->p1), &(nhs->p2), &(hs->p2),
		&(hs->c), fabs (znorm), P13_CODE);
      if ((slot > 0) && (prev_znorm * znorm < 0.0)) {
	db_plane_add_edge (depth_count - 2, FALSE);
	db_plane_add_edge (depth_count - 1, TRUE);
      }
      prev_znorm = znorm;
    }

  } else {			/* not colourparts */

    colour *c1 = &(current_state->planecolour);
    colour *c2 = &(current_state->plane2colour);

    for (slot = 0; slot < helix_segment_count - 1; slot++) {
      hs = helix_segments + slot;
      nhs = hs + 1;
      v3_middle (&pos, &(hs->p1), &(hs->p2));
      v3_difference (&dir1, &(nhs->p1), &pos);
      v3_difference (&dir2, &(nhs->p2), &pos);
      v3_cross_product (&normal, &dir2, &dir1);
      v3_normalize (&normal);
      znorm = normal.z / sin (acos (nhs->a.z));
      db_plane (&(hs->p1), &(nhs->p1), &(nhs->p2), &(hs->p2),
		((znorm > 0.0) ? c1 : c2), fabs (znorm), P13_CODE);
      if ((slot > 0) && (prev_znorm * znorm < 0.0)) {
	db_plane_add_edge (depth_count - 2, FALSE);
	db_plane_add_edge (depth_count - 1, TRUE);
      }
      prev_znorm = znorm;
    }
  }

  db_plane_add_edge (first, TRUE);
  db_plane_add_edge (depth_count - 1, FALSE);
}


/*------------------------------------------------------------*/
void
ps_label (vector3 *p, char *label, colour *c)
{
  label_db_entry *lae;

  assert (p);
  assert (label);
  assert (*label);
  assert (label_array);

  if (label_count >= label_alloc) {
    label_alloc *= 2;
    label_array = realloc (label_array, label_alloc * sizeof (label_db_entry));
  }
  lae = label_array + label_count;

  v3_sum (&(lae->pos), p, &(current_state->labeloffset));
  lae->str = str_clone (label);
  if (c) {
    lae->col = *c;
  } else {
    lae->col = current_state->linecolour;
  }

  enter_depth (LABEL_CODE, label_count, lae->pos.z);

  label_count++;
}


/*------------------------------------------------------------*/
void
ps_line (boolean polylines)
{
  vector3 *prev, *curr;
  colour *col;
  line_segment *ls;
  int slot;

  if (line_segment_count < 2) return;

  if (current_state->colourparts) {

    if (polylines) {

      prev = &(line_segments[0].p);
      col = &(line_segments[0].c);

      for (slot = 1; slot < line_segment_count; slot++) {
	ls = line_segments + slot;
	curr = &(ls->p);
	if (! ls->new) db_line (prev, curr, col);
	prev = curr;
	col = &(ls->c);
      }

    } else {			/* not polylines */

      for (slot = 0; slot < line_segment_count; slot += 2) {
	ls = line_segments + slot;
	prev = &(ls->p);
	col = &(ls->c);
	ls++;
	curr = &(ls->p);
	db_line (prev, curr, col);
      }
    }

  } else {			/* not colourparts */

    if (polylines) {

      prev = &(line_segments->p);

      for (slot = 1; slot < line_segment_count; slot++) {
	ls = line_segments + slot;
	curr = &(ls->p);
	if (! ls->new) db_line (prev, curr, NULL);
	prev = curr;
      }

    } else {			/* not polylines */

      for (slot = 0; slot < line_segment_count; slot += 2) {
	prev = &(line_segments[slot].p);
	curr = &(line_segments[slot+1].p);
	db_line (prev, curr, NULL);
      }

    }
  }
}


/*------------------------------------------------------------*/
void
ps_sphere (at3d *at, double radius)
{
  assert (at);
  assert (radius > 0.0);

  db_sphere (&(at->xyz), radius, &(at->colour), TRUE);
}


/*------------------------------------------------------------*/
void
ps_stick (vector3 *v1, vector3 *v2, double r1, double r2, colour *c)
{
  vector3 vec, coo1, coo2, perp;
  double radius, shorten1, shorten2, depth;

  assert (v1);
  assert (v2);
  assert (v3_distance (v1, v2) > 0.0);

  if (outside_extent_radius (v1, current_state->stickradius) &&
      outside_extent_radius (v2, current_state->stickradius) &&
      outside_extent_2v (v1, v2)) return;

  v3_difference (&vec, v2, v1);
				/* skip if hidden by either sphere */
  if (sqrt (vec.x * vec.x + vec.y * vec.y) < ((r1 < r2) ? r1 : r2)) return;
  v3_normalize (&vec);

  if (acos (fabs (vec.z)) > to_radians (84.0)) {/* if stick flat on xy plane */
				                /* then output as plane */
    vector3 p1, p2, p3, p4;
				/* reduce radius a bit, depending on taper */
    radius = (1.0 - current_state->sticktaper * 0.2) *
             current_state->stickradius;
				/* shorten stick by sphere radii */
    shorten1 = (r1 > radius) ? sqrt (r1 * r1 - radius * radius) : 0.0;
    shorten2 = (r2 > radius) ? sqrt (r2 * r2 - radius * radius) : 0.0;
				/* skip if shortened to negative length */
    if (shorten1 + shorten2 >= v3_distance (v1, v2)) return;
				/* shorten end points proj'ed onto xy plane */
    vec.z = 0.0;
    v3_normalize (&vec);
    v3_sum_scaled (&coo1, v1, shorten1, &vec);
    v3_sum_scaled (&coo2, v2, -shorten2, &vec);

    v3_cross_product (&perp, &vec, &zaxis);
    v3_normalize (&perp);
    v3_scale (&perp, radius);
    v3_sum (&p1, &coo1, &perp);
    v3_sum (&p2, &coo2, &perp);
    v3_difference (&p3, &coo2, &perp);
    v3_difference (&p4, &coo1, &perp);

    if (c) {
      db_plane (&p1, &p2, &p3, &p4, c, 1.0, P1234_CODE);
    } else {
      db_plane (&p1, &p2, &p3, &p4,
		&(current_state->planecolour), 1.0, P1234_CODE);
    }

    shorten1 = v1->z + 0.5 * r1; /* adjust depth setting; kludge */
    shorten2 = v2->z + 0.5 * r2;
    (depth_array + depth_count - 1)->depth =
      ((shorten1 > shorten2) ? shorten1 : shorten2) + 0.001;

  } else {			/* out of xy plane: output as proper stick */

    stick_db_entry *ste;

    radius = current_state->stickradius;
				/* shorten stick by sphere radii */
    shorten1 = (r1 > radius) ? sqrt (r1 * r1 - radius * radius) : 0.0;
    shorten2 = (r2 > radius) ? sqrt (r2 * r2 - radius * radius) : 0.0;
				/* skip if shortened to negative length */
    if (shorten1 + shorten2 >= v3_distance (v1, v2)) return;

    if (stick_count >= stick_alloc) {
      stick_alloc *= 2;
      stick_array = realloc (stick_array, stick_alloc * sizeof (stick_db_entry));
    }
    ste = stick_array + stick_count;

    if (v1->z > v2->z) {	/* closest point first */
      ste->p1 = *v1;
      v3_scale (&vec, shorten2);
      v3_difference (&(ste->p2), v2, &vec);
    } else {
      ste->p1 = *v2;
      v3_scale (&vec, shorten1);
      v3_sum (&(ste->p2), v1, &vec);
    }
				/* direction vector, and perpendicular */
    v3_difference (&(ste->dir), &(ste->p2), &(ste->p1));
    v3_normalize (&(ste->dir));
    v3_cross_product (&(ste->perp), &zaxis, &(ste->dir));
    v3_normalize (&(ste->perp));
    v3_scale (&(ste->perp), current_state->stickradius);

    if (c) {
      ste->col = *c;
    } else {
      ste->col = current_state->planecolour;
    }

    ste->taper = (1.0 - current_state->sticktaper) +
	         current_state->sticktaper *
	         acos (fabs (ste->dir.z)) / to_radians (90.0);

    if (r1 < 0.0) {		/* colourparts; split stick */
      ste->taper = 0.5 + 0.5 * ste->taper;
      if (v1->z > v2->z) {
	v3_scale (&(ste->perp), ste->taper);
	depth = v2->z + 0.5 * r2 + 0.001;
      } else {
	depth = v2->z + 0.5 * r2 - 0.001;
      }
    } else if (r2 < 0.0) {
      ste->taper = 0.5 + 0.5 * ste->taper;
      if (v1->z < v2->z) {
	v3_scale (&(ste->perp), ste->taper);
	depth = v1->z + 0.5 * r1 + 0.001;
      } else {
	depth = v1->z + 0.5 * r1 - 0.001;
      }
    } else {
      depth = 0.5 * (v1->z + 0.5 * r1 + v2->z + 0.5 * r2);
    }

    enter_depth (STICK_CODE, stick_count, depth);

    stick_count++;
  }
}


/*------------------------------------------------------------*/
static double
plane_znorm (vector3 *p1, vector3 *p2, vector3 *p3, vector3 *p4, double znorm)
{
  assert (p1);
  assert (p2);
  assert (p3);
  assert (p4);

  if (znorm >= 0.0) {
    return znorm;

  } else {
    vector3 dir1, dir2, across, n1, n2;

    v3_difference (&dir1, p2, p1);
    v3_difference (&dir2, p3, p4);
    v3_sum (&across, p1, p2);
    v3_subtract (&across, p3);
    v3_subtract (&across, p4);
    v3_cross_product (&n1, &across, &dir1);
    v3_cross_product (&n2, &across, &dir2);

    if ((n1.z >= 0.0) || (n2.z >= 0.0)) {
      db_line (p1, p4, NULL);
      db_line (p2, p3, NULL);
      p1->z -= 0.01;
      p2->z -= 0.01;
      p3->z -= 0.01;
      p4->z -= 0.01;
      return 0.0;
    } else {
      return znorm;
    }
  }
}


/*------------------------------------------------------------*/
void
ps_strand (void)
{
  int slot;
  vector3 normal, pos;
  int thickness = current_state->strandthickness >= 0.01;
  strand_segment *ss, *nss;
  colour *col;
				/* strand faces, either colour */
  ss = strand_segments;
  nss = ss + 1;

  if (current_state->colourparts) {
    col = &(ss->c);		/* colourparts: set for every segment */
  } else {
    col = &(current_state->planecolour); /* single colour: set only once */
  }

  v3_sum (&normal, &(ss->n1), &(nss->n1));
  v3_normalize (&normal);
  if (thickness) {
    db_plane (&(ss->p1), &(nss->p1), &(nss->p4), &(ss->p4), col,
	      plane_znorm (&(ss->p1), &(nss->p1),
			   &(nss->p4), &(ss->p4), normal.z), P134_CODE);
    db_plane (&(ss->p2), &(nss->p2), &(nss->p3), &(ss->p3), col,
	      plane_znorm (&(ss->p2), &(nss->p2),
			   &(nss->p3), &(ss->p3), -normal.z), P134_CODE);
  } else {
    db_plane (&(ss->p1), &(nss->p1), &(nss->p4), &(ss->p4),
	      col, fabs (normal.z), P134_CODE);
  }

  for (slot = 1; slot < strand_segment_count - 4; slot++) {
    ss = strand_segments + slot;
    nss = ss + 1;
    if (current_state->colourparts) col = &(ss->c);
    v3_sum (&normal, &(ss->n1), &(nss->n1));
    v3_normalize (&normal);
    if (thickness) {
      db_plane (&(ss->p1), &(nss->p1), &(nss->p4), &(ss->p4), col,
		plane_znorm (&(ss->p1), &(nss->p1),
			     &(nss->p4), &(ss->p4), normal.z), P13_CODE);
      db_plane (&(ss->p2), &(nss->p2), &(nss->p3), &(ss->p3), col,
		plane_znorm (&(ss->p1), &(nss->p1),
			     &(nss->p4), &(ss->p4), -normal.z), P13_CODE);
    } else {
      db_plane (&(ss->p1), &(nss->p1), &(nss->p4), &(ss->p4),
		col, fabs (normal.z), P13_CODE);
    }
  }

  ss = strand_segments + strand_segment_count - 4; /* arrow first part */
  nss = ss + 2;
  if (current_state->colourparts) col = &(ss->c);
  v3_sum (&normal, &(ss->n1), &(nss->n1));
  v3_normalize (&normal);
  if (thickness) {
    v3_middle (&pos, &(nss->p1), &(nss->p4));
    db_tri (&(ss->p1), &(ss->p4), &pos, col, normal.z, T_CODE);
    v3_middle (&pos, &(nss->p2), &(nss->p3));
    db_tri (&(ss->p2), &(ss->p3), &pos, col, -normal.z, T_CODE);
    v3_middle (&pos, &(nss->p1), &(nss->p4));
    db_plane (&((ss+1)->p1), &(nss->p1), &pos, &(ss->p1),
	      col, normal.z, P14_CODE);
    db_plane (&((ss+1)->p4), &(nss->p4), &pos, &(ss->p4),
	      col, normal.z, P14_CODE);
    v3_middle (&pos, &(nss->p2), &(nss->p3));
    db_plane (&((ss+1)->p2), &(nss->p2), &pos, &(ss->p2),
	      col, -normal.z, P14_CODE);
    db_plane (&((ss+1)->p3), &(nss->p3), &pos, &(ss->p3),
	      col, -normal.z, P14_CODE);
  } else {
    v3_middle (&pos, &(nss->p1), &(nss->p4));
    db_tri (&(ss->p1), &(ss->p4), &pos, col, fabs (normal.z), T_CODE);
    v3_middle (&pos, &(nss->p1), &(nss->p4));
    db_plane (&((ss+1)->p1), &(nss->p1), &pos, &(ss->p1),
	      col, fabs (normal.z), P14_CODE);
    db_plane (&((ss+1)->p4), &(nss->p4), &pos, &(ss->p4),
	      col, fabs (normal.z), P14_CODE);
  }

  ss = strand_segments + strand_segment_count - 2; /* arrow tip */
  nss = ss + 1;
  if (current_state->colourparts) col = &(ss->c);
  v3_sum (&normal, &(ss->n1), &(nss->n1));
  v3_normalize (&normal);
  if (thickness) {
    db_plane (&(ss->p1), &(nss->p1), &(nss->p1), &(ss->p4),
	      col, normal.z, P13_CODE);
    db_plane (&(ss->p2), &(nss->p2), &(nss->p2), &(ss->p3),
	      col, -normal.z, P13_CODE);
  } else {
    db_plane (&(ss->p1), &(nss->p1), &(nss->p1), &(ss->p4),
	      &(current_state->planecolour), fabs (normal.z), P13_CODE);
  }

  if (thickness) {
    vector3 dir1, dir2;
				/* strand base, either colour */
    ss = strand_segments;
    if (current_state->colourparts) {
      col = &(ss->c);		/* colourparts: set for every segment */
    } else {
      col = &(current_state->plane2colour); /* single colour: set only once */
    }
    v3_difference (&dir1, &(ss->p3), &(ss->p2));
    v3_difference (&dir2, &(ss->p1), &(ss->p2));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);

    db_plane (&(ss->p1), &(ss->p2), &(ss->p3), &(ss->p4),
	      col, normal.z, P1234_CODE);

				/* strand sides, either colour */
    nss = ss + 1;
    v3_sum (&normal, &(ss->n2), &(nss->n2));
    v3_normalize (&normal);
    db_plane (&(ss->p1), &(nss->p1), &(nss->p2), &(ss->p2),
	      col, normal.z, P134_CODE);
    db_plane (&(ss->p4), &(nss->p4), &(nss->p3), &(ss->p3),
	      col, -normal.z, P134_CODE);

    for (slot = 1; slot < strand_segment_count - 5; slot++) {
      ss = strand_segments + slot;
      nss = ss + 1;
      if (current_state->colourparts) col = &(ss->c);
      v3_sum (&normal, &(ss->n2), &(nss->n2));
      v3_normalize (&normal);
      db_plane (&(ss->p1), &(nss->p1), &(nss->p2), &(ss->p2), col,
		plane_znorm (&(ss->p1), &(nss->p1),
			     &(nss->p2), &(ss->p2), normal.z), P13_CODE);
      db_plane (&(ss->p4), &(nss->p4), &(nss->p3), &(ss->p3), col,
		plane_znorm (&(ss->p4), &(nss->p4),
			     &(nss->p3), &(ss->p3), -normal.z), P13_CODE);
    }

    ss = strand_segments + strand_segment_count - 5;
    nss = ss + 1;
    if (current_state->colourparts) col = &(ss->c);
    v3_sum (&normal, &(ss->n2), &(nss->n2));
    v3_normalize (&normal);
    db_plane (&(ss->p1), &(nss->p1), &(nss->p2), &(ss->p2),
	      col, normal.z, P123_CODE);
    db_plane (&(ss->p4), &(nss->p4), &(nss->p3), &(ss->p3),
	      col, -normal.z, P123_CODE);

				/* arrow base */
    ss = strand_segments + strand_segment_count - 4;
    nss = ss + 1;
    if (current_state->colourparts) col = &(ss->c);
    v3_difference (&dir1, &(ss->p3), &(ss->p1));
    v3_difference (&dir2, &(ss->p4), &(ss->p2));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);
    db_plane (&(ss->p1), &(nss->p1), &(nss->p2), &(ss->p2),
	      col, normal.z, P1234_CODE);
    db_plane (&(ss->p4), &(nss->p4), &(nss->p3), &(ss->p3),
	      col, normal.z, P1234_CODE);

				/* arrow sides, first part */
    ss = strand_segments + strand_segment_count - 3;
    nss = ss + 1;
    if (current_state->colourparts) col = &(ss->c);
    v3_difference (&dir1, &(ss->p2), &(nss->p1));
    v3_difference (&dir2, &(ss->p1), &(nss->p2));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);
    db_plane (&(ss->p1), &(nss->p1), &(nss->p2), &(ss->p2),
	      col, normal.z, P134_CODE);
    v3_difference (&dir1, &(ss->p4), &(nss->p3));
    v3_difference (&dir2, &(ss->p3), &(nss->p4));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);
    db_plane (&(ss->p4), &(nss->p4), &(nss->p3), &(ss->p3),
	      col, normal.z, P134_CODE);

				/* arrow sides, last part */
    ss = strand_segments + strand_segment_count - 2;
    nss = ss + 1;
    if (current_state->colourparts) col = &(ss->c);
    v3_difference (&dir1, &(ss->p2), &(nss->p1));
    v3_difference (&dir2, &(ss->p1), &(nss->p2));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);
    db_plane (&(ss->p1), &(nss->p1), &(nss->p2), &(ss->p2),
	      col, normal.z, P123_CODE);

    v3_difference (&dir1, &(ss->p4), &(nss->p2));
    v3_difference (&dir2, &(ss->p3), &(nss->p1));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);
    db_plane (&(ss->p4), &(nss->p1), &(nss->p2), &(ss->p3),
	      col, normal.z, P123_CODE);
  }
}


/*------------------------------------------------------------*/
void
ps_object (int code, vector3 *triplets, int count)
{
  int slot;
  vector3 *v;
  vector3 normal;
  colour rgb = {COLOUR_RGB, 1.0, 1.0, 1.0};

  assert (triplets);
  assert (count > 0);

  switch (code) {

  case OBJ_POINTS:
    for (slot = 0; slot < count; slot++) db_point (triplets + slot, NULL);
    break;

  case OBJ_POINTS_COLOURS:
    for (slot = 0; slot < count; slot++) {
      v = triplets + slot + 1;
      rgb.x = v->x;
      rgb.y = v->y;
      rgb.z = v->z;
      db_point (--v, &rgb);
    }
    break;

  case OBJ_LINES:
    for (slot = 1; slot < count; slot++) {
      db_line (triplets + slot - 1, triplets + slot, NULL);
    }
    break;

  case OBJ_LINES_COLOURS:
    for (slot = 2; slot < count; slot += 2) {
      v = triplets + slot - 1;
      rgb.x = v->x;
      rgb.y = v->y;
      rgb.z = v->z;
      db_line (v - 1, v + 1, &rgb);
    }
    break;

  case OBJ_TRIANGLES:
    for (slot = 0; slot < count; slot += 3) {
      v = triplets + slot;
      v3_triangle_normal (&normal, v, v + 1, v + 2);
      db_tri (v, v + 1, v + 2,
	      &(current_state->planecolour), fabs (normal.z), T_CODE);
    }
    break;

  case OBJ_TRIANGLES_COLOURS:
    for (slot = 0; slot < count; slot += 6) {
      v = triplets + slot + 5;
      rgb.x = v->x;
      rgb.y = v->y;
      rgb.z = v->z;
      v = triplets + slot;
      v3_triangle_normal (&normal, v, v + 2, v + 4);
      db_tri (v, v + 2, v + 4, &rgb, fabs (normal.z), T_CODE);
    }
    break;

  case OBJ_TRIANGLES_NORMALS:
    for (slot = 0; slot < count; slot += 6) {
      v = triplets + slot;
      v3_sum (&normal, v + 1, v + 3);
      v3_add (&normal, v + 5);
      v3_normalize (&normal);
      db_tri (v, v + 2, v + 4,
	      &(current_state->planecolour), fabs (normal.z), T_CODE);
    }
    break;

  case OBJ_TRIANGLES_NORMALS_COLOURS:
    for (slot = 0; slot < count; slot += 9) {
      v = triplets + slot + 8;
      rgb.x = v->x;
      rgb.y = v->y;
      rgb.z = v->z;
      v = triplets + slot;
      v3_sum (&normal, v + 1, v + 4);
      v3_add (&normal, v + 7);
      v3_normalize (&normal);
      db_tri (v, v + 3, v + 6, &rgb, fabs (normal.z), T_CODE);
    }
    break;

  case OBJ_STRIP:
    for (slot = 2; slot < count; slot++) {
      v = triplets + slot;
      v3_triangle_normal (&normal, v - 2, v - 1, v);
      db_tri (v - 2, v - 1, v,
	      &(current_state->planecolour), fabs (normal.z), T_CODE);
    }
    break;

  case OBJ_STRIP_COLOURS:
    for (slot = 4; slot < count; slot += 2) {
      v = triplets + slot + 1;
      rgb.x = v->x;
      rgb.y = v->y;
      rgb.z = v->z;
      v = triplets + slot;
      v3_triangle_normal (&normal, v - 4, v - 2, v);
      db_tri (v - 4, v - 2, v, &rgb, fabs (normal.z), T_CODE);
    }
    break;

  case OBJ_STRIP_NORMALS:
    for (slot = 4; slot < count; slot += 2) {
      v = triplets + slot;
      v3_sum (&normal, v - 3, v - 1);
      v3_add (&normal, v + 1);
      v3_normalize (&normal);
      db_tri (v - 4, v - 2, v,
	      &(current_state->planecolour), fabs (normal.z), T_CODE);
    }
    break;

  case OBJ_STRIP_NORMALS_COLOURS:
    for (slot = 6; slot < count; slot += 3) {
      v = triplets + slot + 2;
      rgb.x = v->x;
      rgb.y = v->y;
      rgb.z = v->z;
      v = triplets + slot;
      v3_sum (&normal, v - 5, v - 2);
      v3_add (&normal, v + 1);
      v3_normalize (&normal);
      db_tri (v - 6, v - 3, v, &rgb, fabs (normal.z), T_CODE);
    }
    break;

  }
}
