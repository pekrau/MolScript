/* graphics.c

   MolScript v2.1.2

   Graphics: construct the geometries and call the output procedures.

   Copyright (C) 1997-1998 Per Kraulis
     6-Dec-1996  first attempts
    10-Oct-1997  fairly finished
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "clib/str_utils.h"
#include "clib/angle.h"
#include "clib/extent3d.h"
#include "clib/hermite_curve.h"
#include "clib/matrix3.h"

#include "graphics.h"
#include "global.h"
#include "segment.h"
#include "lex.h"
#include "select.h"
#include "xform.h"

				/* empirically determined factors */
#define HELIX_HERMITE_FACTOR 4.7
#define STRAND_HERMITE_FACTOR 0.5
#define HELIX_ALPHA (to_radians(32.0))
#define HELIX_BETA (to_radians(-11.0))


/*============================================================*/
boolean frame;
double area [4];
colour background_colour;
double window;
double slab;
boolean headlight;
boolean shadows;
double fog;

double aspect_ratio;
double aspect_window_x, aspect_window_y;


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void (*output_first_plot) (void);
void (*output_finish_output) (void);
void (*output_start_plot) (void);
void (*output_finish_plot) (void);

void (*set_area) (void);
void (*set_background) (void);
void (*anchor_start) (char *str);
void (*anchor_description) (char *str);
void (*anchor_parameter) (char *str);
void (*anchor_start_geometry) (void);
void (*anchor_finish) (void);
void (*lod_start) (void);
void (*lod_finish) (void);
void (*lod_start_group) (void);
void (*lod_finish_group) (void);
void (*viewpoint_start) (char *str);
void (*viewpoint_output) (void);
void (*output_directionallight) (void);
void (*output_pointlight) (void);
void (*output_spotlight) (void);
void (*output_comment) (char *str);

void (*output_coil) (void);
void (*output_cylinder) (vector3 *v1, vector3 *v2);
void (*output_helix) (void);
void (*output_label) (vector3 *p, char *label, colour *c);
void (*output_line) (boolean polylines);
void (*output_sphere) (at3d *at, double radius);
void (*output_stick) (vector3 *v1, vector3 *v2,
		      double r1, double r2,  colour *c);
void (*output_strand) (void);

void (*output_start_object) (void);
void (*output_object) (int code, vector3 *triplets, int count);
void (*output_finish_object) (void);

void (*output_pickable) (at3d *atom);


/*------------------------------------------------------------*/
static void
msg_chain (char *type, mol3d_chain *ch)
{
  assert (type);
  assert (ch);

  if (message_mode)
    fprintf (stderr, "%s from %s to %s (%i residues)\n", type,
	     (ch->residues[0])->name, (ch->residues[ch->length-1])->name,
	     ch->length);
}


/*------------------------------------------------------------*/
static void
set_aspect_ratio (void)
{
  aspect_ratio = (area[3] - area[1]) / (area[2] - area[0]);
  if (aspect_ratio >= 1.0) {
    aspect_window_x = window;
    aspect_window_y = aspect_ratio * window;
  } else {
    aspect_window_x = window / aspect_ratio;
    aspect_window_y = window;
  }
}


/*------------------------------------------------------------*/
void
graphics_plot_init (void)
{
  frame = TRUE;
  window = -1.0;
  slab = -1.0;
  headlight = TRUE;
  shadows = FALSE;
  fog = 0.0;
  ext3d_initialize();
}


/*------------------------------------------------------------*/
void
set_area_values (double xlo, double ylo, double xhi, double yhi)
{
  area[0] = xlo;
  area[1] = ylo;
  area[2] = xhi;
  area[3] = yhi;
  set_aspect_ratio();
}


/*------------------------------------------------------------*/
void
set_window (void)
{
  assert (dstack_size == 1);

  if (dstack[0] <= 0.0) {
    yyerror ("invalid window value");
    return;
  }

  window = dstack[0] / 2.0;
  clear_dstack();

  set_aspect_ratio();

  assert (dstack_size == 0);
}


/*------------------------------------------------------------*/
void
set_slab (void)
{
  assert (dstack_size == 1);

  if (dstack[0] <= 0.0) {
    yyerror ("invalid slab value");
    return;
  }

  slab = dstack[0] / 2.0;
  clear_dstack();

  assert (dstack_size == 0);
}


/*------------------------------------------------------------*/
void
set_fog (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 0.0) {
    yyerror ("invalid fog value");
    return;
  }

  fog = dstack[0];
  clear_dstack();

  assert (dstack_size == 0);
}


/*------------------------------------------------------------*/
void
set_extent (void)
{
  vector3 center, size;
  double d;

  ext3d_get_center_size (&center, &size);
  v3_scale (&size, 0.5);

  if (window < 0.0) {
    if (aspect_ratio < 1.0) {
      size.x *= aspect_ratio;
    } else {
      size.y /= aspect_ratio;
    }
    window = fabs (center.x - size.x);
    d = fabs (center.x + size.x);
    if (d > window) window = d;
    d = fabs (center.y - size.y);
    if (d > window) window = d;
    d = fabs (center.y + size.y);
    if (d > window) window = d;
    window += 2.0;		/* make fit less snug */
    if (message_mode)
      fprintf (stderr, "setting window to %.2f\n", 2.0 * window);
  }

  if (slab < 0.0) {
    slab = fabs (center.z - size.z);
    d = fabs (center.z + size.z);
    if (d > slab) slab = d;
    if (message_mode)
      fprintf (stderr, "setting slab to %.2f\n", 2.0 * slab);
  }
}


/*------------------------------------------------------------*/
int
outside_extent_radius (vector3 *v, double radius)
{
  assert (v);
  assert (radius >= 0.0);

  if (window > 0.0) {
    if (v->x + radius < -aspect_window_x) return TRUE;
    if (v->x - radius > aspect_window_x) return TRUE;
    if (v->y + radius < -aspect_window_y) return TRUE;
    if (v->y - radius > aspect_window_y) return TRUE;
  }

  if (slab > 0.0) {
    if (v->z + radius < -slab) return TRUE;
    if (v->z - radius > slab) return TRUE;
  }

  return FALSE;
}


/*------------------------------------------------------------*/
int
outside_extent_2v (vector3 *v1, vector3 *v2)
{
  assert (v1);
  assert (v2);

  if (window > 0.0) {
    if ((v1->x < -aspect_window_x) && (v2->x < -aspect_window_x)) return TRUE;
    if ((v1->x > aspect_window_x) && (v2->x > aspect_window_x)) return TRUE;
    if ((v1->y < -aspect_window_y) && (v2->y < -aspect_window_y)) return TRUE;
    if ((v1->y > aspect_window_y) && (v2->y > aspect_window_y)) return TRUE;
  }

  if (slab > 0.0) {
    if ((v1->z < -slab) && (v2->z < -slab)) return TRUE;
    if ((v1->z > slab) && (v2->z > slab)) return TRUE;
  }

  return FALSE;
}


/*------------------------------------------------------------*/
double
depthcue (double depth, state *st)
{
  double factor;

  assert (st);

  if (slab <= 0.0) return 1.0;

  factor = depth / slab;
  if (factor < -1.0) {
    factor = -1.0;
  } else if (factor > 1.0) {
    factor = 1.0;
  }
  factor = (factor / 2.0 - 0.5) * st->depthcue + 1.0;

  return factor;
}


/*------------------------------------------------------------*/
void
ball_and_stick (int single_selection)
{
  at3d **atoms1, **atoms2;
  int atom_count1, atom_count2, slot1, slot2, start;
  vector3 *v1, *v2;
  double radius, dist;

  if (single_selection) {

    assert (count_atom_selections() == 1);

    atoms1 = select_atom_list (&atom_count1);

    if (message_mode)
      fprintf (stderr, "%i atoms selected for ball-and-stick\n", atom_count1);

    if (atoms1 == NULL) return;

    for (slot1 = 0; slot1 < atom_count1; slot1++) { /* balls output */
      radius = 0.25 * atoms1[slot1]->radius;
      if (radius > 0.0) {
	output_sphere (atoms1[slot1], radius);
	ext3d_update (&(atoms1[slot1]->xyz), radius);
      }
    }

    atoms2 = atoms1;
    atom_count2 = atom_count1;

  } else {

    assert (count_atom_selections() == 2);

    atoms1 = select_atom_list (&atom_count1);
    atoms2 = select_atom_list (&atom_count2);

    if (message_mode) {
      fprintf (stderr, "%i atoms in first set and %i atoms in second set for ball-and-stick\n", atom_count1, atom_count2);
    }

    if (atoms1 == NULL) {
      if (atoms2 != NULL) free (atoms2);
      return;
    } else if (atoms2 == NULL) {
      free (atoms1);
      return;
    }
  }

  assert (count_atom_selections() == 0);

  if (current_state->colourparts) { /* sticks output, atom colour */

    vector3 middle;

    for (slot1 = 0; slot1 < atom_count1; slot1++) {
      v1 = &(atoms1[slot1]->xyz);

      if (single_selection) {
	start = slot1 + 1;
      } else {
	start = 0;
      }

      for (slot2 = start; slot2 < atom_count2; slot2++) {
	if (atoms1[slot1] == atoms2[slot2]) continue;

	v2 = &(atoms2[slot2]->xyz);

	dist = v3_distance (v1, v2);
	if ((dist > current_state->bonddistance) || (dist < 0.001)) continue;

	if (colour_unequal (&(atoms1[slot1]->colour),
			    &(atoms2[slot2]->colour))) {
	  v3_middle (&middle, v1, v2);
	  output_stick (v1, &middle, 0.25 * atoms1[slot1]->radius, -1.0,
			&(atoms1[slot1]->colour));
	  output_stick (&middle, v2, -1.0, 0.25 * atoms2[slot2]->radius,
			&(atoms2[slot2]->colour));
	} else {
	  output_stick (v1, v2,
			0.25 * atoms1[slot1]->radius,
			0.25 * atoms2[slot2]->radius,
			&(atoms1[slot1]->colour));
	}
	ext3d_update (v1, current_state->stickradius);
	ext3d_update (v2, current_state->stickradius);
      }
    }

  } else {			/* sticks output, overall colour */

    for (slot1 = 0; slot1 < atom_count1; slot1++) {
      v1 = &(atoms1[slot1]->xyz);

      if (single_selection) {
	start = slot1 + 1;
      } else {
	start = 0;
      }

      for (slot2 = start; slot2 < atom_count2; slot2++) {
	if (atoms1[slot1] == atoms2[slot2]) continue;

	v2 = &(atoms2[slot2]->xyz);

	dist = v3_distance (v1, v2);
	if ((dist > current_state->bonddistance) || (dist < 0.001)) continue;

	output_stick (v1, v2,
		      0.25 * atoms1[slot1]->radius,
		      0.25 * atoms2[slot2]->radius,
		      NULL);
	ext3d_update (v1, current_state->stickradius);
	ext3d_update (v2, current_state->stickradius);
      }
    }
  }

  if (single_selection) {
    free (atoms1);
  } else {
    free (atoms2);
    free (atoms1);
  }

  assert (count_atom_selections() == 0);
}


/*------------------------------------------------------------*/
void
bonds (int single_selection)
{
  at3d **atoms1, **atoms2;
  int atom_count1, atom_count2, slot1, slot2, start;
  vector3 *v1, *v2;
  colour *col1;
  line_segment *ls;
  double dist;

  if (single_selection) {

    assert (count_atom_selections() == 1);
    atoms1 = select_atom_list (&atom_count1);
    assert (count_atom_selections() == 0);

    if (message_mode)
      fprintf (stderr, "%i atoms selected for bonds\n", atom_count1);

    if (atoms1 == NULL) return;

    atoms2 = atoms1;
    atom_count2 = atom_count1;

    for (slot1 = 0; slot1 < atom_count1; slot1++) atoms1[slot1]->rval = 0.0;

  } else {

    assert (count_atom_selections() == 2);
    atoms1 = select_atom_list (&atom_count1);
    atoms2 = select_atom_list (&atom_count2);
    assert (count_atom_selections() == 0);

    if (message_mode) {
      fprintf (stderr,
	       "%i atoms in first set and %i atoms in second set for bonds\n",
	       atom_count1, atom_count2);
    }

    if (atoms1 == NULL) {
      if (atoms2 != NULL) free (atoms2);
      return;
    } else if (atoms2 == NULL) {
      free (atoms1);
      return;
    }
  }

  line_segment_init();

  if (current_state->colourparts) {
    vector3 middle;

    for (slot1 = 0; slot1 < atom_count1; slot1++) {
      v1 = &(atoms1[slot1]->xyz);
      col1 = &(atoms1[slot1]->colour);

      if (single_selection) {
	start = slot1 + 1;
      } else {
	start = 0;
      }

      for (slot2 = start; slot2 < atom_count2; slot2++) {
	v2 = &(atoms2[slot2]->xyz);
	if (v2 == v1) continue;

	dist = v3_distance (v1, v2);
	if ((dist > current_state->bonddistance) || (dist < 0.001)) continue;

	if (colour_unequal (col1, &(atoms2[slot2]->colour))) {
	  v3_middle (&middle, v1, v2);

	  ls = line_segment_next();
	  ls->p = *v1;
	  ls->c = *col1;
	  ls = line_segment_next();
	  ls->p = middle;

	  ls = line_segment_next();
	  ls->p = middle;
	  ls->c = atoms2[slot2]->colour;
	  ls = line_segment_next();
	  ls->p = *v2;

	} else {
	  ls = line_segment_next();
	  ls->p = *v1;
	  ls->c = *col1;
	  ls = line_segment_next();
	  ls->p = *v2;
	}

	ext3d_update (v1, 0.0);
	ext3d_update (v2, 0.0);

	atoms1[slot1]->rval++;
	atoms2[slot2]->rval++;
      }
    }

  } else {

    for (slot1 = 0; slot1 < atom_count1; slot1++) {
      v1 = &(atoms1[slot1]->xyz);

      if (single_selection) {
	start = slot1 + 1;
      } else {
	start = 0;
      }

      for (slot2 = start; slot2 < atom_count2; slot2++) {
	v2 = &(atoms2[slot2]->xyz);
	if (v2 == v1) continue;

	dist = v3_distance (v1, v2);
	if ((dist > current_state->bonddistance) || (dist < 0.001)) continue;

	ls = line_segment_next();
	ls->p = *v1;
	ls = line_segment_next();
	ls->p = *v2;

	ext3d_update (v1, 0.0);
	ext3d_update (v2, 0.0);
	atoms1[slot1]->rval++;
	atoms2[slot2]->rval++;
      }

      if (line_segment_count > 0) line_segments->c = current_state->linecolour;
    }
  }

  if (single_selection && (current_state->bondcross != 0.0)) {
    double radius = 0.5 * current_state->bondcross;

    if (! current_state->colourparts) col1 = NULL;

    for (slot1 = 0; slot1 < atom_count1; slot1++) {
      if (atoms1[slot1]->rval != 0.0) continue;

      atoms1[slot1]->rval = 1.0;

      v1 = &(atoms1[slot1]->xyz);
      if (current_state->colourparts) col1 = &(atoms1[slot1]->colour);

      ls = line_segment_next();
      v3_sum_scaled (&(ls->p), v1, radius, &xaxis);
      if (col1) ls->c = *col1;
      ls = line_segment_next();
      v3_sum_scaled (&(ls->p), v1, -radius, &xaxis);

      ls = line_segment_next();
      v3_sum_scaled (&(ls->p), v1, radius, &yaxis);
      if (col1) ls->c = *col1;
      ls = line_segment_next();
      v3_sum_scaled (&(ls->p), v1, -radius, &yaxis);

      ls = line_segment_next();
      v3_sum_scaled (&(ls->p), v1, radius, &zaxis);
      if (col1) ls->c = *col1;
      ls = line_segment_next();
      v3_sum_scaled (&(ls->p), v1, -radius, &zaxis);

      ext3d_update (v1, 0.0);
    }
  }

  output_line (FALSE);

  if (output_pickable) {
    if (single_selection) {
      for (slot1 = 0; slot1 < atom_count1; slot1++) {
	if (atoms1[slot1]->rval != 0.0) {
	  output_pickable (atoms1[slot1]);
	}
      }
    } else {
    }
  }

  if (single_selection) {
    free (atoms1);
  } else {
    free (atoms2);
    free (atoms1);
  }

  assert (count_atom_selections() == 0);
}


/*------------------------------------------------------------*/
static vector3 *
get_atom_positions (mol3d_chain *ch)
{
  int slot;
  vector3 *points;

  assert (ch);

  points = malloc (ch->length * sizeof (vector3));
  for (slot = 0; slot < ch->length; slot++) {
    *(points + slot) = ch->atoms[slot]->xyz;
  }

  return points;
}


/*------------------------------------------------------------*/
static void
priestle_smoothing (vector3 *points, int length, int steps)
{
  int sm, slot;
  vector3 *ptmp = malloc (length * sizeof (vector3));

  assert (points);

  for (sm = 0; sm < steps; sm++) {

    for (slot = 1; slot < length-1; slot++) {
      v3_middle (ptmp + slot, points + slot - 1, points + slot + 1);
      v3_middle (ptmp + slot, ptmp + slot, points + slot);
    }

    for (slot = 1; slot < length-1; slot++) {
      *(points + slot) = *(ptmp + slot);
    }
  }

  free (ptmp);
}


/*------------------------------------------------------------*/
void
coil (int is_peptide_chain, int smoothing)
{
  char *atomname, *coilname;
  double chain_distance, t;
  mol3d_chain *first_ch, *ch;
  vector3 *points;
  res3d *res;
  at3d *first, *last;
  vector3 vec1, vec2;
  colour *col;
  int slot, segment;
  double hermite_factor = 0.5 * current_state->splinefactor;
  int segments = current_state->segments;

  assert (count_residue_selections() == 1);

				/* colourparts requires even number of seg's */
  if (current_state->colourparts && (segments % 2)) segments++;

  if (is_peptide_chain) {
    atomname = PEPTIDE_CHAIN_ATOMNAME;
    chain_distance = PEPTIDE_DISTANCE;
    first_ch = get_peptide_chains();
    if (smoothing) {
      coilname = "coil";
    } else {
      coilname = "turn";
    }

  } else {
    atomname = NUCLEOTIDE_CHAIN_ATOMNAME;
    chain_distance = NUCLEOTIDE_DISTANCE;
    coilname = "double-helix";
    first_ch = get_nucleotide_chains();
  }

  for (ch = first_ch; ch; ch = ch->next) {
    if (ch->length < 2) continue;

    msg_chain (coilname, ch);
    points = get_atom_positions (ch);

    first = NULL;		/* find atom before or in first residue */
    res = ch->residues[0]->prev;
    if (res) {
      first = at3d_lookup (res, atomname);
      if (first) {
	if (v3_distance (&(first->xyz), points) >=
	    chain_distance) first = NULL;
      }
    }
    if (first == NULL) first = ch->atoms[0];

    last = NULL;		/* find atom after or in last residue */
    res = ch->residues[ch->length-1]->next;
    if (res) {
      last = at3d_lookup (res, atomname);
      if (last) {
	if (v3_distance (&(last->xyz), points + ch->length-1) >=
	    chain_distance) last = NULL;
      }
    }
    if (last == NULL) last = ch->atoms[ch->length-1];

    if (smoothing)
      priestle_smoothing (points, ch->length, current_state->smoothsteps);

    if (current_state->colourparts) {
      col = &(ch->residues[0]->colour);
    } else {
      col = NULL;
    }

    if (current_state->coilradius < 0.01) { /* coil rendered as line */
      line_segment *ls;

      line_segment_init();
      ls = line_segment_next();
      ls->new = TRUE;
      ls->p = *points;
      if (col) {
	ls->c = *col;
      } else {
	ls->c = current_state->planecolour;
      }
      ext3d_update (points, 0.0);

      v3_difference (&vec2, points + 1, &(first->xyz));
      if (first == ch->atoms[0]) v3_scale (&vec2, 2.0);
      v3_scale (&vec2, hermite_factor);

      for (slot = 0; slot < ch->length - 1; slot++) {

	vec1 = vec2;
	if (slot == ch->length - 2) {
	  v3_difference (&vec2, &(last->xyz), points + ch->length - 2);
	  if (last == ch->atoms[ch->length-1]) v3_scale (&vec2, 2.0);
	} else {
	  v3_difference (&vec2, points + slot + 2, points + slot);
	}
	v3_scale (&vec2, hermite_factor);

	hermite_set (points + slot, points + slot + 1, &vec1, &vec2);
	for (segment = 1; segment < segments; segment++) {
	  t = ((double) segment) / ((double) segments);
	  ls = line_segment_next();
	  hermite_get (&(ls->p), t);
	  if (current_state->colourparts &&
	      (segment == segments / 2) &&
	      colour_unequal (col, &(ch->residues[slot+1]->colour))) {
	    ls->c = *col;
	    ls = line_segment_next();
	    ls->new = TRUE;
	    ls->p = (ls - 1)->p;
	    col = &(ch->residues[slot+1]->colour);
	  }
	  if (col) ls->c = *col;
	  ext3d_update (&(ls->p), 0.0);
	}

	ls = line_segment_next();
	ls->p = *(points + slot + 1);
	if (col) ls->c = *col;
	ext3d_update (points + slot + 1, 0.0);
      }

      output_line (TRUE);

    } else {			/* coil rendered as solid */
      coil_segment *cs;
      vector3 side = {1.0, 0.0, 0.0};
      vector3 normal, dir;
      double radius = current_state->coilradius / sqrt (2.0);

      coil_segment_init();	/* coil path and colour*/
      cs = coil_segment_next();
      cs->p = *points;
      if (col) {
	cs->c = *col;
      } else {
	cs->c = current_state->planecolour;
      }
      ext3d_update (&(cs->p), radius);

      v3_difference (&vec2, points + 1, &(first->xyz));
      if (first == ch->atoms[0]) v3_scale (&vec2, 2.0);
      v3_scale (&vec2, hermite_factor);

      for (slot = 0; slot < ch->length - 1; slot++) {

	vec1 = vec2;
	if (slot == ch->length - 2) {
	  v3_difference (&vec2, &(last->xyz), points + ch->length - 2);
	  if (last == ch->atoms[ch->length-1]) v3_scale (&vec2, 2.0);
	} else {
	  v3_difference (&vec2, points + slot + 2, points + slot);
	}
	v3_scale (&vec2, hermite_factor);

	hermite_set (points + slot, points + slot + 1, &vec1, &vec2);
	for (segment = 1; segment < segments; segment++) {
	  t = ((double) segment) / ((double) segments);
	  cs = coil_segment_next();
	  hermite_get (&(cs->p), t);
	  if (current_state->colourparts &&
	      (segment == segments / 2)) col = &(ch->residues[slot+1]->colour);
	  if (col) cs->c = *col;
	  ext3d_update (&(cs->p), radius);
	}

	cs = coil_segment_next();
	cs->p = *(points +slot + 1);
	if (col) cs->c = *col;
	ext3d_update (points + slot + 1, radius);
      }
				/* coil plane coordinates */
      v3_difference (&dir, &(coil_segments[1].p), &(coil_segments[0].p));
      v3_normalize (&dir);
      v3_cross_product (&normal, &side, &dir);
      if (v3_length (&normal) < 1.0e-6) {
	v3_initialize (&side, 0.0, 1.0, 0.0);
	v3_cross_product (&normal, &side, &dir);
	if (v3_length (&normal) < 1.0e-6) {
	  v3_initialize (&side, 0.0, 0.0, 1.0);
	  v3_cross_product (&normal, &side, &dir);
	}
      }
      v3_normalize (&normal);
      v3_cross_product (&side, &dir, &normal);
      v3_normalize (&side);

      cs = coil_segments;
      v3_sum_scaled (&(cs->p1), &(cs->p), radius, &normal);
      v3_add_scaled (&(cs->p1), radius, &side);
      v3_sum_scaled (&(cs->p2), &(cs->p), -radius, &normal);
      v3_add_scaled (&(cs->p2), radius, &side);
      v3_sum_scaled (&(cs->p3), &(cs->p), -radius, &normal);
      v3_add_scaled (&(cs->p3), -radius, &side);
      v3_sum_scaled (&(cs->p4), &(cs->p), radius, &normal);
      v3_add_scaled (&(cs->p4), -radius, &side);

      for (slot = 1; slot < coil_segment_count - 1; slot++) {
	v3_difference (&dir, &(coil_segments[slot+1].p),
		             &(coil_segments[slot-1].p));
	v3_cross_product (&side, &dir, &normal);
	v3_normalize (&side);
	v3_cross_product (&normal, &side, &dir);
	v3_normalize (&normal);

	cs = coil_segments + slot;
	v3_sum_scaled (&(cs->p1), &(cs->p), radius, &normal);
	v3_add_scaled (&(cs->p1), radius, &side);
	v3_sum_scaled (&(cs->p2), &(cs->p), -radius, &normal);
	v3_add_scaled (&(cs->p2), radius, &side);
	v3_sum_scaled (&(cs->p3), &(cs->p), -radius, &normal);
	v3_add_scaled (&(cs->p3), -radius, &side);
	v3_sum_scaled (&(cs->p4), &(cs->p), radius, &normal);
	v3_add_scaled (&(cs->p4), -radius, &side);
      }

      v3_difference (&dir, &(coil_segments[coil_segment_count-1].p),
		           &(coil_segments[coil_segment_count-2].p));
      v3_cross_product (&side, &dir, &normal);
      v3_normalize (&side);
      v3_cross_product (&normal, &side, &dir);
      v3_normalize (&normal);

      cs = coil_segments + coil_segment_count - 1;
      v3_sum_scaled (&(cs->p1), &(cs->p), radius, &normal);
      v3_add_scaled (&(cs->p1), radius, &side);
      v3_sum_scaled (&(cs->p2), &(cs->p), -radius, &normal);
      v3_add_scaled (&(cs->p2), radius, &side);
      v3_sum_scaled (&(cs->p3), &(cs->p), -radius, &normal);
      v3_add_scaled (&(cs->p3), -radius, &side);
      v3_sum_scaled (&(cs->p4), &(cs->p), radius, &normal);
      v3_add_scaled (&(cs->p4), -radius, &side);

				/* coil plane normals */
      for (slot = 0; slot < coil_segment_count; slot++) {
	cs = coil_segments + slot;
	v3_difference (&(cs->n1), &(cs->p1), &(cs->p));
	v3_normalize (&(cs->n1));
	v3_difference (&(cs->n2), &(cs->p2), &(cs->p));
	v3_normalize (&(cs->n2));
	v3_difference (&(cs->n3), &(cs->p3), &(cs->p));
	v3_normalize (&(cs->n3));
	v3_difference (&(cs->n4), &(cs->p4), &(cs->p));
	v3_normalize (&(cs->n4));
      }

      output_coil();
    }

    free (points);
  }

  if (first_ch) mol3d_chain_delete (first_ch);

  assert (count_residue_selections() == 0);
}


/*------------------------------------------------------------*/
void
cpk (void)
{
  at3d **atoms;
  int atom_count, slot;
  double radius;

  assert (count_atom_selections() == 1);

  atoms = select_atom_list (&atom_count);

  assert (count_atom_selections() == 0);

  if (message_mode)
    fprintf (stderr, "%i atoms selected for cpk\n", atom_count);
  if (atoms == NULL) return;

  for (slot = 0; slot < atom_count; slot++) {
    radius = atoms[slot]->radius;
    if (radius > 0.0) {
      output_sphere (atoms[slot], radius);
      ext3d_update (&(atoms[slot]->xyz), radius);
    }
  }

  free (atoms);
}


/*------------------------------------------------------------*/
void
cylinder (void)
{
  mol3d_chain *first_ch, *ch;
  vector3 start, finish, dir, vec, top;

  assert (count_residue_selections() == 1);

  first_ch = get_peptide_chains();

  for (ch = first_ch; ch; ch = ch->next) {
    if (ch->length < 3) continue;

    msg_chain ("cylinder", ch);

    v3_middle (&start, &(ch->atoms[0]->xyz), &(ch->atoms[2]->xyz));
    v3_middle (&finish, &(ch->atoms[ch->length-3]->xyz),
		             &(ch->atoms[ch->length-1]->xyz));

    if (ch->length == 3) {	/* compute helix axis as for ordinary helix */
      vector3 vec1, vec2, rvec;

      v3_difference (&vec, &(ch->atoms[2]->xyz), &(ch->atoms[0]->xyz));
      v3_normalize (&vec);

      v3_difference (&vec1, &(ch->atoms[1]->xyz), &(ch->atoms[0]->xyz));
      v3_difference (&vec2, &(ch->atoms[2]->xyz), &(ch->atoms[1]->xyz));
      v3_cross_product (&rvec, &vec1, &vec2);
      v3_normalize (&rvec);

      v3_scaled (&dir, cos (HELIX_ALPHA), &rvec);
      v3_add_scaled (&dir, sin (HELIX_ALPHA), &vec);

    } else {			/* compute helix axis from terminii coords */
      v3_difference (&dir, &finish, &start);
    }

    v3_normalize (&dir);

    v3_difference (&vec, &(ch->atoms[2]->xyz), &(ch->atoms[0]->xyz));
    v3_scaled (&top, - 0.5 * v3_length (&vec) *
		          cos (v3_angle (&vec, &dir)), &dir);
    v3_add (&start, &top);

    v3_difference (&vec, &(ch->atoms[ch->length-1]->xyz),
			      &(ch->atoms[ch->length-3]->xyz));
    v3_scaled (&top, 0.5 * v3_length (&vec) *
		          cos (v3_angle (&vec, &dir)), &dir);
    v3_add (&finish, &top);

    output_cylinder (&start, &finish);
    ext3d_update (&start, current_state->cylinderradius);
    ext3d_update (&finish, current_state->cylinderradius);
  }

  if (first_ch) mol3d_chain_delete (first_ch);

  assert (count_residue_selections() == 0);
}


/*------------------------------------------------------------*/
void
helix (void)
{
  mol3d_chain *first_ch, *ch;
  vector3 *points, *axes, *tangents;
  vector3 cvec, rvec, vec1, vec2, pos, dir;
  res3d *res;
  at3d *ca_first, *ca_last, *at;
  int slot, segment;
  double halfwidth, t;
  colour *col;
  helix_segment *hs;
  double coilradius = current_state->coilradius;
  int segments = current_state->segments;

  assert (count_residue_selections() == 1);

  first_ch = get_peptide_chains();

				/* colourparts requires even number of seg's */
  if (current_state->colourparts && (segments % 2)) segments++;

  if (coilradius < 0.01) coilradius = 0.01; /* not too thin */

  for (ch = first_ch; ch; ch = ch->next) {
    if (ch->length < 3) continue;

    msg_chain ("helix", ch);

    points = get_atom_positions (ch); /* helix axis and tangent vectors */
    axes = malloc (ch->length * sizeof (vector3));
    tangents = malloc (ch->length * sizeof (vector3));

    for (slot = 1; slot < ch->length - 1; slot++) {
				/* helix direction vector at (i) */
      v3_difference (&cvec, points + slot + 1, points + slot - 1);
      v3_normalize (&cvec);
				/* normal vector for plane (i-1),(i),(i+1) */
      v3_difference (&vec1, points + slot, points + slot - 1);
      v3_difference (&vec2, points + slot + 1, points + slot);
      v3_cross_product (&rvec, &vec1, &vec2);
      v3_normalize (&rvec);
				/* helix axis at (i) */
      v3_scaled (&vec1, cos (HELIX_ALPHA), &rvec);
      v3_scaled (&vec2, sin (HELIX_ALPHA), &cvec);
      v3_sum (axes + slot, &vec1, &vec2);
				/* helix tangent at (i) */
      v3_scaled (&vec1, cos (HELIX_BETA), &cvec);
      v3_scaled (&vec2, sin (HELIX_BETA), &rvec);
      v3_sum (tangents + slot, &vec1, &vec2);
      v3_scale (tangents + slot, HELIX_HERMITE_FACTOR);
    }

    ca_first = ch->atoms[0];	/* find CA before and after chain, if any */
    res = ch->residues[0]->prev; /* used for helix terminii tangents */
    if (res) {
      at = at3d_lookup (res, PEPTIDE_CHAIN_ATOMNAME);
      if (at &&
	  v3_distance (points, &(at->xyz)) <= PEPTIDE_DISTANCE) ca_first = at;
    }

    ca_last = ch->atoms[ch->length-1];
    res = ch->residues[ch->length-1]->next;
    if (res) {
      at = at3d_lookup (res, PEPTIDE_CHAIN_ATOMNAME);
      if (at &&
	  v3_distance (points + ch->length - 1,
		       &(at->xyz)) <= PEPTIDE_DISTANCE) ca_last = at;
    }
				/* helix terminii axes */
    *(axes) = *(axes + 1);
    *(axes + ch->length - 1) = *(axes + ch->length - 2);
				/* helix terminii tangents */
    v3_difference (&vec1, points + 1, &(ca_first->xyz));
    v3_normalize (&vec1);
    v3_scaled (tangents, HELIX_HERMITE_FACTOR, &vec1);
    v3_difference (&vec1, &(ca_last->xyz), points + ch->length - 2);
    v3_normalize (&vec1);
    v3_scaled (tangents + ch->length - 1, HELIX_HERMITE_FACTOR, &vec1);

    if (current_state->colourparts) {
      col = &(ch->residues[0]->colour);
    } else {
      col = NULL;
    }

    helix_segment_init();
    hs = helix_segment_next();
    v3_sum_scaled (&(hs->p1), points, coilradius, axes);
    v3_sum_scaled (&(hs->p2), points, -coilradius, axes);
    hs->a = *axes;
    v3_cross_product (&(hs->n), tangents, axes);
    v3_normalize (&(hs->n));
    if (col) hs->c = *col;
    ext3d_update (points, coilradius);

				/* helix start segments */
    pos = *points;
    hermite_set (points, points + 1, tangents, tangents + 1);

    for (segment = 1; segment < segments; segment++) {
      t = ((double) segment) / ((double) segments);
      hermite_get (&pos, t);
      halfwidth = coilradius +
	(0.5 * current_state->helixwidth - coilradius) *
	0.5 * (- cos (ANGLE_PI * t) + 1.0);
      hs = helix_segment_next();
      v3_sum_scaled (&(hs->p1), &pos, halfwidth, axes);
      v3_sum_scaled (&(hs->p2), &pos, -halfwidth, axes);
      hs->a = *axes;
      hermite_get_tangent (&dir, t);
      v3_cross_product (&(hs->n), &dir, axes);
      v3_normalize (&(hs->n));
      if (current_state->colourparts &&
	  (segment == segments / 2)) col = &(ch->residues[1]->colour);
      if (col) hs->c = *col;
      ext3d_update (&pos, halfwidth);
    }

    halfwidth = 0.5 * current_state->helixwidth;
    hs = helix_segment_next();
    v3_sum_scaled (&(hs->p1), points + 1, halfwidth, axes + 1);
    v3_sum_scaled (&(hs->p2), points + 1, -halfwidth, axes + 1);
    hs->a = *(axes + 1);
    v3_cross_product (&(hs->n), tangents + 1, axes + 1);
    v3_normalize (&(hs->n));
    if (col) hs->c = *col;
    ext3d_update (points + 1, halfwidth);

				/* helix main segments */
    for (slot = 1; slot < ch->length - 2; slot++) {

      hermite_set (points + slot, points + slot + 1,
		   tangents + slot, tangents + slot + 1);

      for (segment = 1; segment < segments; segment++) {
	t = ((double) segment) / ((double) segments);
	hs = helix_segment_next();
	v3_scaled (&(hs->a), 1.0 - t, axes + slot);
	v3_add_scaled (&(hs->a), t, axes + slot + 1);
	v3_normalize (&(hs->a));
	hermite_get (&pos, t);
	v3_sum_scaled (&(hs->p1), &pos, halfwidth, &(hs->a));
	v3_sum_scaled (&(hs->p2), &pos, -halfwidth, &(hs->a));
	hermite_get_tangent (&dir, t);
	v3_cross_product (&(hs->n), &dir, &(hs->a));
	v3_normalize (&(hs->n));
	if (current_state->colourparts && (segment == segments / 2))
	  col = &(ch->residues[slot+1]->colour);
	if (col) hs->c = *col;
	ext3d_update (&pos, halfwidth);
      }

      hs = helix_segment_next();
      v3_sum_scaled (&(hs->p1), points + slot + 1, halfwidth, axes + slot + 1);
      v3_sum_scaled (&(hs->p2), points + slot + 1, -halfwidth, axes + slot +1);
      hs->a = *(axes + slot + 1);
      v3_cross_product (&(hs->n), tangents + slot + 1, axes + slot + 1);
      v3_normalize (&(hs->n));
      if (col) hs->c = *col;
      ext3d_update (points + slot + 1, halfwidth);
    }
				/* helix finish segments */
    pos = *(points + ch->length - 2);
    hermite_set (points + ch->length - 2, points + ch->length - 1,
		 tangents + ch->length - 2, tangents + ch->length - 1);

    for (segment = 1; segment < segments; segment++) {
      t = ((double) segment) / ((double) segments);
      hermite_get (&pos, t);
      halfwidth = coilradius +
	(0.5 * current_state->helixwidth - coilradius) *
	0.5 * (cos (ANGLE_PI * t) + 1.0);
      hs = helix_segment_next();
      v3_sum_scaled (&(hs->p1), &pos, halfwidth, axes + ch->length - 1);
      v3_sum_scaled (&(hs->p2), &pos, -halfwidth, axes + ch->length - 1);
      hs->a = *(axes + ch->length - 1);
      hermite_get_tangent (&dir, t);
      v3_cross_product (&(hs->n), &dir, axes + ch->length - 1);
      v3_normalize (&(hs->n));
      if (current_state->colourparts && (segment == segments / 2))
	  col = &(ch->residues[ch->length-2]->colour);
      if (col) hs->c = *col;
      ext3d_update (&pos, halfwidth);
    }

    hs = helix_segment_next();
    v3_sum_scaled (&(hs->p1), points + ch->length - 1,
		              coilradius, axes + ch->length - 1);
    v3_sum_scaled (&(hs->p2), points + ch->length - 1,
		              -coilradius, axes + ch->length - 1);
    hs->a = *(axes + ch->length - 1);
    v3_cross_product (&(hs->n),
		      tangents + ch->length - 1, axes + ch->length - 1);
    v3_normalize (&(hs->n));
    if (col) hs->c = *col;
    ext3d_update (points + ch->length - 1, coilradius);

    output_helix();

    free (points);
    free (axes);
    free (tangents);
  }

  if (first_ch) mol3d_chain_delete (first_ch);

  assert (count_residue_selections() == 0);
}


/*------------------------------------------------------------*/
void
line_start (void)
{
  assert (dstack_size == 3);

  line_segment_init();
  line_next();
  line_segments->new = TRUE;

  assert (dstack_size == 0);
}


/*------------------------------------------------------------*/
void
line_next (void)
{
  line_segment *ls;

  assert (dstack_size == 3);

  ls = line_segment_next();
  ls->p.x = dstack[0];
  ls->p.y = dstack[1];
  ls->p.z = dstack[2];
  clear_dstack();
  ls->c = current_state->linecolour;

  ext3d_update (&(ls->p), 0.0);

  assert (dstack_size == 0);
}


/*------------------------------------------------------------*/
static void
apply_labelmask_case (char *str, int len)
{
  int slot;
  int last = (len > current_state->labelmasklength) ?
             current_state->labelmasklength : len;
  int *mask = current_state->labelmask;

  assert (str);
  assert (len > 0);

  for (slot = 0; slot < last; slot++) {
    switch (mask[slot] % 3) {
    case 1:
      *str = tolower (*str);
      break;
    case 2:
      *str = toupper (*str);
      break;
    default:
      break;
    }
    str++;
  }
}


/*------------------------------------------------------------*/
void
label_atoms (char *label)
{
  at3d *at;
  at3d **atoms;
  char *orig_str, *copy_str;
  int slot, length, rpos, tpos, cpos, apos, atom_count;

  assert (count_atom_selections() == 1);
  assert (label);
  assert (*label);

  length = strlen (label) + 4 + 2 - 1 + 2  + 1; /* max possible length */
  orig_str = malloc (length * sizeof (char));
  copy_str = malloc (length * sizeof (char));

  rpos = -1;
  tpos = -1;
  cpos = -1;
  apos = -1;
  length = 0;
  slot = 0;
  while (slot < strlen (label)) {

    if (label[slot] == '%') {
      slot++;
      if (slot >= strlen (label)) goto skip;

      switch (label[slot]) {
      case 'r':
	if (rpos >= 0) goto skip;
	rpos = length;
	length += 6;
	break;
      case 't':
	if (tpos >= 0) goto skip;
	tpos = length;
	length += 4;
	break;
      case 'c':
	if (cpos >= 0) goto skip;
	cpos = length;
	length += 1;
	break;
      case 'a':
	if (apos >= 0) goto skip;
	apos = length;
	length += 4;
	break;
      default:
	goto skip;
      }
    } else {
      orig_str[length] = label[slot];
      length++;
    }

    slot++;
  }

  atoms = select_atom_list (&atom_count);

  assert (count_atom_selections() == 0);

  if (message_mode)
    fprintf (stderr, "%i atoms selected for label\n", atom_count);

  for (slot = 0; slot < atom_count; slot++) {

    at = atoms[slot];

    if (rpos >= 0) strncpy (orig_str + rpos, at->res->name, 6);
    if (tpos >= 0) strncpy (orig_str + tpos, at->res->type, 4);
    if (cpos >= 0) *(orig_str + cpos) = at->res->code;
    if (apos >= 0) strncpy (orig_str + apos, at->name, 4);

    memcpy (copy_str, orig_str, length * sizeof (char));
    apply_labelmask_case (copy_str, length);
    str_remove_nulls (copy_str, length);

    if (current_state->colourparts) {
      output_label (&(at->xyz), copy_str, &(at->colour));
    } else {
      output_label (&(at->xyz), copy_str, NULL);
    }
    ext3d_update (&(at->xyz), 0.0);
  }

  free (copy_str);
  free (orig_str);
  free (atoms);

  return;

 skip:
  yyerror ("invalid label format string");
}


/*------------------------------------------------------------*/
void
label_position (char *label)
{
  vector3 pos;

  assert (dstack_size == 3);
  assert (label);
  assert (*label);

  pos.x = dstack[0];
  pos.y = dstack[1];
  pos.z = dstack[2];
  clear_dstack();

  apply_labelmask_case (label, strlen (label));
  output_label (&pos, label, NULL);
  ext3d_update (&pos, 0.0);
}


/*------------------------------------------------------------*/
void
object (char *filename)
{
  FILE *file;
  char object_code[4];
  int close_file, code, alloc, slot, count, number;
  vector3 *triplets;
  int total = 0;

  if (filename) {
    file = fopen (filename, "r");
    if (file == NULL) {
      yyerror ("could not open the object file");
      return;
    }
    close_file = TRUE;
  } else {
    file = lex_input_file();
    close_file = FALSE;
  }

  alloc = 1024;
  triplets = malloc (alloc * sizeof (vector3));

  output_start_object();

  while (fscanf (file, "%3s", object_code) != EOF) {
    if (str_eq (object_code, "P")) {
      code = OBJ_POINTS;
    } else if (str_eq (object_code, "PC")) {
      code = OBJ_POINTS_COLOURS;
    } else if (str_eq (object_code, "L")) {
      code = OBJ_LINES;
    } else if (str_eq (object_code, "LC")) {
      code = OBJ_LINES_COLOURS;
    } else if (str_eq (object_code, "T")) {
      code = OBJ_TRIANGLES;
    } else if (str_eq (object_code, "TC")) {
      code = OBJ_TRIANGLES_COLOURS;
    } else if (str_eq (object_code, "TN")) {
      code = OBJ_TRIANGLES_NORMALS;
    } else if (str_eq (object_code, "TNC")) {
      code = OBJ_TRIANGLES_NORMALS_COLOURS;
    } else if (str_eq (object_code, "S")) {
      code = OBJ_STRIP;
    } else if (str_eq (object_code, "SC")) {
      code = OBJ_STRIP_COLOURS;
    } else if (str_eq (object_code, "SN")) {
      code = OBJ_STRIP_NORMALS;
    } else if (str_eq (object_code, "SNC")) {
      code = OBJ_STRIP_NORMALS_COLOURS;
    } else if (str_eq (object_code, "Q")) {
      goto finish;
    } else {
      goto format_error;
    }

    if (fscanf (file, "%i", &count) != 1) goto format_error;

    switch (code) {
    case OBJ_POINTS:
      if (count < 1) goto format_error;
      number = 1;
      break;
    case OBJ_POINTS_COLOURS:
      if (count < 1) goto format_error;
      number = 2;
      break;
    case OBJ_LINES:
      if (count < 2) goto format_error;
      number = 1;
      break;
    case OBJ_LINES_COLOURS:
      if (count < 2) goto format_error;
      number = 2;
      break;
    case OBJ_TRIANGLES:
      if ((count < 3) || (count % 3 != 0)) goto format_error;
      number = 1;
      break;
    case OBJ_TRIANGLES_COLOURS:
    case OBJ_TRIANGLES_NORMALS:
      if ((count < 3) || (count % 3 != 0)) goto format_error;
      number = 2;
      break;
    case OBJ_TRIANGLES_NORMALS_COLOURS:
      if ((count < 3) || (count % 3 != 0)) goto format_error;
      number = 3;
      break;
    case OBJ_STRIP:
      if (count < 3) goto format_error;
      number = 1;
      break;
    case OBJ_STRIP_COLOURS:
    case OBJ_STRIP_NORMALS:
      if (count < 3) goto format_error;
      number = 2;
      break;
    case OBJ_STRIP_NORMALS_COLOURS:
      if (count < 3) goto format_error;
      number = 3;
      break;
    default:
      goto format_error;
    }

    count *= number;

    if (count > alloc) {
      alloc = count;
      triplets = realloc (triplets, count * sizeof (vector3));
    }

    for (slot = 0; slot < count; slot++) {
      if (fscanf (file, "%lg %lg %lg",
		  &(triplets[slot].x),
		  &(triplets[slot].y),
		  &(triplets[slot].z)) != 3) goto format_error;
    }

    if (current_state->objecttransform) {
      for (slot = 0; slot < count; slot += number) { /* coordinate xform */
	matrix3_transform (triplets + slot, xform);
      }
      if (code == OBJ_TRIANGLES_NORMALS || /* normals rotate, not xform */
	  code == OBJ_TRIANGLES_NORMALS_COLOURS ||
	  code == OBJ_STRIP_NORMALS ||
	  code == OBJ_STRIP_NORMALS_COLOURS) {
	for (slot = 1; slot < count; slot += number) {
	  matrix3_rotate (triplets + slot, xform);
	}
      }
    }

    for (slot = 0; slot < count; slot += number) {
      ext3d_update (triplets + slot, 0.0);
    }

    output_object (code, triplets, count);
    total += count;
  }

finish:
  free (triplets);
  output_finish_object();
  if (close_file) fclose (file);
  if (message_mode)
    fprintf (stderr, "%i data triplets read from object file\n", total);
  return;

format_error:
  free (triplets);
  output_finish_object();
  if (close_file) fclose (file);
  yyerror ("invalid format or content in object file");
}


/*------------------------------------------------------------*/
void
strand (void)
{
  mol3d_chain *first_ch, *ch;
  vector3 *points, *normals, *smoothed;
  vector3 dir, dir1, dir2, pos, side, vec1, vec2, normal;
  colour *col;
  strand_segment *ss;
  int slot, segment, segments;
  double thickness, radius, t;
  double width = current_state->strandwidth / 2.0;

  assert (count_residue_selections() == 1);

  if (current_state->colourparts) {         /* strand is less curved; */
    segments = current_state->segments / 2; /* colourparts requires */
    if (segments % 2 != 0) segments++;      /* even number of segments */
  } else {
    segments = current_state->segments / 2 + 1;
  }

  if (current_state->strandthickness > current_state->strandwidth) {
    radius = current_state->strandthickness / 2.0;
  } else {
    radius = current_state->strandwidth / 2.0;
  }

  if (current_state->strandthickness < 0.01) {
    thickness = 0.0;
  } else {
    thickness = current_state->strandthickness / 2.0;
  }

  first_ch = get_peptide_chains();

  for (ch = first_ch; ch; ch = ch->next) {
    if (ch->length < 3) continue;

    msg_chain ("strand", ch);

    points = get_atom_positions (ch);
				/* normals for the strand */
    normals = malloc (ch->length * sizeof (vector3));
    for (slot = 1; slot < ch->length - 1; slot++) {
      v3_middle (&pos, points + slot - 1, points + slot + 1);
      v3_difference (normals + slot, points + slot, &pos);
      v3_normalize (normals + slot);
    }
				/* just copy the normals for the terminii */
    *(normals) = *(normals + 1);
    *(normals + ch->length - 1) = *(normals + ch->length - 2);

				/* smooth CA positions */
    priestle_smoothing (points, ch->length, current_state->smoothsteps);

				/* normals must point the same way */
    for (slot = 0; slot < ch->length - 1; slot++) {
      if (v3_dot_product (normals + slot, normals + slot + 1) < 0.0) {
	v3_reverse (normals + slot + 1);
      }
    }
				/* smooth normals, one iteration */
    smoothed = malloc (ch->length * sizeof (vector3));
    for (slot = 1; slot < ch->length - 1; slot++) {
      v3_sum (&dir, normals + slot - 1, normals + slot);
      v3_add (&dir, normals + slot + 1);
      v3_normalize (&dir);
      *(smoothed + slot) = dir;
    }
    for (slot = 1; slot < ch->length - 1; slot++) {
      *(normals + slot) = *(smoothed + slot);
    }
    free (smoothed);

				/* normals exactly perpendicular to strand */
    v3_difference (&dir, points + 1, points);
    v3_cross_product (&side, &dir, normals);
    v3_cross_product (normals, &side, &dir);
    v3_normalize (normals);
    for (slot = 1; slot < ch->length - 1; slot++) {
      v3_difference (&dir, points + slot + 1, points + slot - 1);
      v3_cross_product (&side, &dir, normals + slot);
      v3_cross_product (normals + slot, &side, &dir);
      v3_normalize (normals + slot);
    }
    v3_difference (&dir, points + ch->length - 1, points + ch->length - 2);
    v3_cross_product (&side, &dir, normals + ch->length - 1);
    v3_cross_product (normals + ch->length - 1, &side, &dir);
    v3_normalize (normals + ch->length - 1);

    if (current_state->colourparts) {
      col = &(ch->residues[0]->colour);
    } else {
      col = NULL;
    }

    strand_segment_init();
				/* strand body */
    v3_difference (&dir2, points + 1, points);
    v3_normalize (&dir2);
    v3_difference (&vec2, points + 1, points);
    v3_scale (&vec2, STRAND_HERMITE_FACTOR);

    for (slot = 0; slot < ch->length - 2; slot++) {

      dir1 = dir2;		/* direction vectors */
      v3_difference (&dir2, points + slot + 2, points + slot);
      v3_normalize (&dir2);
      vec1 = vec2;		/* Hermite spline vectors */
      v3_difference (&vec2, points + slot + 2, points + slot);
      v3_scale (&vec2, STRAND_HERMITE_FACTOR);
      hermite_set (points + slot, points + slot + 1, &vec1, &vec2);

      for (segment = 0; segment < segments; segment++) {
	t = ((double) segment) / ((double) segments);
	hermite_get (&pos, t);
	v3_scaled (&dir, 1.0 - t, &dir1);
	v3_add_scaled (&dir, t, &dir2);
	v3_scaled (&normal, 1.0 - t, normals + slot);
	v3_add_scaled (&normal, t, normals + slot + 1);
	v3_cross_product (&side, &normal, &dir);
	v3_normalize (&side);
	if (current_state->colourparts &&
	    (segment == segments / 2)) col = &(ch->residues[slot+1]->colour);
	ss = strand_segment_next();
	v3_sum_scaled (&(ss->p1), &pos, width, &side);
	v3_add_scaled (&(ss->p1), thickness, &normal);
	v3_sum_scaled (&(ss->p2), &pos, width, &side);
	v3_add_scaled (&(ss->p2), -thickness, &normal);
	v3_sum_scaled (&(ss->p3), &pos, -width, &side);
	v3_add_scaled (&(ss->p3), -thickness, &normal);
	v3_sum_scaled (&(ss->p4), &pos, -width, &side);
	v3_add_scaled (&(ss->p4), thickness, &normal);
	ss->n1 = normal;
	ss->n2 = side;
	v3_scaled (&(ss->n3), -1.0, &normal);
	v3_scaled (&(ss->n4), -1.0, &side);
	if (col) ss->c = *col;
	ext3d_update (&pos, radius);
      }
    }
				/* strand body, last segment */
    dir = dir2;
    normal = *(normals + ch->length - 2);
    v3_cross_product (&side, &normal, &dir);
    v3_normalize (&side);
    pos = *(points + ch->length - 2);
    ss = strand_segment_next();
    v3_sum_scaled (&(ss->p1), &pos, width, &side);
    v3_add_scaled (&(ss->p1), thickness, &normal);
    v3_sum_scaled (&(ss->p2), &pos, width, &side);
    v3_add_scaled (&(ss->p2), -thickness, &normal);
    v3_sum_scaled (&(ss->p3), &pos, -width, &side);
    v3_add_scaled (&(ss->p3), -thickness, &normal);
    v3_sum_scaled (&(ss->p4), &pos, -width, &side);
    v3_add_scaled (&(ss->p4), thickness, &normal);
    ss->n1 = normal;
    ss->n2 = side;
    v3_scaled (&(ss->n3), -1.0, &normal);
    v3_scaled (&(ss->n4), -1.0, &side);
    if (col) ss->c = *col;
    ext3d_update (&pos, radius);
				/* strand arrow head: 3 segments */
    ss = strand_segment_next();
    v3_sum_scaled (&(ss->p1), &pos, 1.5 * width, &side);
    v3_add_scaled (&(ss->p1), thickness, &normal);
    v3_sum_scaled (&(ss->p2), &pos, 1.5 * width, &side);
    v3_add_scaled (&(ss->p2), -thickness, &normal);
    v3_sum_scaled (&(ss->p3), &pos, -1.5 * width, &side);
    v3_add_scaled (&(ss->p3), -thickness, &normal);
    v3_sum_scaled (&(ss->p4), &pos, -1.5 * width, &side);
    v3_add_scaled (&(ss->p4), thickness, &normal);
    ss->n1 = normal;
    ss->n2 = side;
    v3_scaled (&(ss->n3), -1.0, &normal);
    v3_scaled (&(ss->n4), -1.0, &side);
    if (col) ss->c = *col;
    ext3d_update (&pos, 1.5 * radius);

    dir1 = dir2;
    v3_difference (&dir2, points + ch->length - 1, points + ch->length - 2);
    v3_normalize (&dir1);
    v3_middle (&dir, &dir1, &dir2);
    v3_middle (&pos, points + ch->length - 2, points + ch->length - 1);
    v3_middle (&normal, normals + ch->length - 2, normals + ch->length - 1);
    v3_normalize (&normal);
    v3_cross_product (&side, &normal, &dir);
    v3_normalize (&side);
    ss = strand_segment_next();
    v3_sum_scaled (&(ss->p1), &pos, 0.75 * width, &side);
    v3_add_scaled (&(ss->p1), thickness, &normal);
    v3_sum_scaled (&(ss->p2), &pos, 0.75 * width, &side);
    v3_add_scaled (&(ss->p2), -thickness, &normal);
    v3_sum_scaled (&(ss->p3), &pos, -0.75 * width, &side);
    v3_add_scaled (&(ss->p3), -thickness, &normal);
    v3_sum_scaled (&(ss->p4), &pos, -0.75 * width, &side);
    v3_add_scaled (&(ss->p4), thickness, &normal);
    ss->n1 = normal;
    ss->n2 = side;
    v3_scaled (&(ss->n3), -1.0, &normal);
    v3_scaled (&(ss->n4), -1.0, &side);
    if (current_state->colourparts) ss->c = ch->residues[ch->length-1]->colour;
    ext3d_update (&pos, 0.75 * radius);

    pos = *(points + ch->length - 1);
    normal = *(normals + ch->length - 1);
    ss = strand_segment_next();
    v3_sum_scaled (&(ss->p1), &pos, thickness, &normal);
    v3_sum_scaled (&(ss->p2), &pos, -thickness, &normal);
    ss->n1 = normal;
    v3_scaled (&(ss->n2), -1.0, &normal);
    ext3d_update (points + ch->length - 1, thickness);

    output_strand();

    free (normals);
    free (points);
  }

  if (first_ch) mol3d_chain_delete (first_ch);

  assert (count_residue_selections() == 0);
}


/*------------------------------------------------------------*/
void
trace (void)
{
  int slot;
  mol3d_chain *first_ch, *ch;
  line_segment *ls;

  assert (count_residue_selections() == 1);

  first_ch = get_peptide_chains();

  line_segment_init();

  for (ch = first_ch; ch; ch = ch->next) {
    if (ch->length < 2) continue;

    ls = line_segment_next();
    ls->new = TRUE;
    ls->p = ch->atoms[0]->xyz;
    ext3d_update (&(ls->p), 0.0);

    if (current_state->colourparts) {
      vector3 middle;

      ls->c = ch->residues[0]->colour;
      for (slot = 1; slot < ch->length; slot++) {
	if (colour_unequal (&(ch->residues[slot-1]->colour),
			    &(ch->residues[slot]->colour))) {
	  ls = line_segment_next();
	  v3_middle (&middle, &(ch->atoms[slot-1]->xyz),
		              &(ch->atoms[slot]->xyz));
	  ls->p = middle;
	  ls->c = ch->residues[slot-1]->colour;
	  ls = line_segment_next();
	  ls->new = TRUE;
	  ls->p = middle;
	  ls->c = ch->residues[slot]->colour;
	}
	ls = line_segment_next();
	ls->p = ch->atoms[slot]->xyz;
	ls->c = ch->residues[slot]->colour;
	ext3d_update (&(ls->p), 0.0);
      }

    } else {
      ls->c = current_state->linecolour;
      for (slot = 1; slot < ch->length; slot++) {
	ls = line_segment_next();
	ls->p = ch->atoms[slot]->xyz;
	ext3d_update (&(ls->p), 0.0);
      }
    }

    if (output_pickable) {
      for (slot = 0; slot < ch->length; slot++) {
	output_pickable (ch->atoms[slot]);
      }
    }

    msg_chain ("trace", ch);
  }

  output_line (TRUE);

  if (first_ch) mol3d_chain_delete (first_ch);

  assert (count_residue_selections() == 0);
}
