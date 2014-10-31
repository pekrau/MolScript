/* state.c

   MolScript v2.1.2

   Graphics state.

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
     2-Jan-1997  basically finished
    26-Apr-1998  push and pop implemented
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "state.h"
#include "global.h"
#include "select.h"


/*============================================================*/
state *current_state = NULL;
static state *current_stack = NULL;


/*------------------------------------------------------------*/
static state *
state_destroy (state *curr)
{
  state *prev;

  assert (curr);

  prev = curr->prev;
  if (curr->labelmask) free (curr->labelmask);
  free (curr);

  return prev;
}


/*------------------------------------------------------------*/
void
state_init (void)
{
  while (current_state) current_state = state_destroy (current_state);
  while (current_stack) current_stack = state_destroy (current_stack);

  current_state = malloc (sizeof (state));
  current_state->prev = NULL;

  current_state->bonddistance = 1.9;
  current_state->bondcross = 0.4;
  current_state->coilradius = 0.2;
  current_state->colourparts = FALSE;
  current_state->cylinderradius = 2.3;
  current_state->depthcue = 0.75;
  current_state->emissivecolour = black_colour;
  current_state->helixthickness = 0.3;
  current_state->helixwidth = 2.4;
  current_state->hsbramp = TRUE;
  current_state->hsbrampreverse = FALSE;
  current_state->labelbackground = 0.0;
  current_state->labelcentre = TRUE;
  current_state->labelclip = FALSE;
  current_state->labelmask = NULL;
  current_state->labelmasklength = 0;
  v3_initialize (&(current_state->labeloffset), 0.0, 0.0, 0.0);
  current_state->labelrotation = FALSE;
  current_state->labelsize = 20.0;
  current_state->lightambientintensity = 0.0;
  v3_initialize (&(current_state->lightattenuation), 1.0, 0.0, 0.0);
  current_state->lightcolour = white_colour;
  current_state->lightintensity = 1.0;
  current_state->lightradius = 100.0;
  if (output_mode == POSTSCRIPT_MODE) {
    current_state->linecolour = black_colour;
  } else {
    current_state->linecolour = white_colour;
  }
  current_state->linedash = 0.0;
  current_state->linewidth = 1.0;
  current_state->objecttransform = TRUE;
  current_state->planecolour = white_colour;
  current_state->plane2colour = grey_colour;
  current_state->regularexpression = FALSE;
  if (output_mode == POSTSCRIPT_MODE || output_mode == RASTER3D_MODE) {
    current_state->segments = 6;
  } else {
    current_state->segments = 3;
  }
  current_state->segmentsize = 2.0;
  current_state->shading = 0.5;
  current_state->shadingexponent = 1.5;
  current_state->shininess = 0.2;
  current_state->smoothsteps = 2;
  if (output_mode == RASTER3D_MODE) {
    current_state->specularcolour = white_colour;
  } else {
    current_state->specularcolour = black_colour;
  }
  current_state->splinefactor = 1.0;
  current_state->stickradius = 0.2;
  current_state->sticktaper = 0.75;
  current_state->strandthickness = 0.6;
  current_state->strandwidth = 2.0;
  current_state->transparency = 0.0;
}


/*------------------------------------------------------------*/
static state *
state_clone (void)
{
  state *new = malloc (sizeof (state));

  memcpy (new, current_state, sizeof (state));
  if (new->labelmasklength > 0) {
    new->labelmask = malloc (new->labelmasklength * sizeof (int));
    memcpy (new->labelmask,
	    current_state->labelmask,
	    new->labelmasklength * sizeof (int));
  } else {
    new->labelmask = NULL;
  }

  return new;
}


/*------------------------------------------------------------*/
void
new_state (void)
{
  state *new = state_clone();

  new->prev = current_state;
  current_state = new;
}


/*------------------------------------------------------------*/
void
push_state (void)
{
  state *new = state_clone();

  new->prev = current_stack;
  current_stack = new;
}


/*------------------------------------------------------------*/
void
pop_state (void)
{
  state *prev;

  if (current_stack == NULL) {
    yyerror ("cannot pop empty state stack");
    return;
  }

  prev = current_stack->prev;
  current_stack->prev = current_state;
  current_state = current_stack;
  current_stack = prev;
}


/*------------------------------------------------------------*/
void
set_atomcolour (void)
{
  mol3d *mol;
  res3d *res;
  at3d *at;
  int *flags;
  int total = 0;

  assert (count_atom_selections() == 1);

  flags = current_atom_sel->flags;
  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) {
	if (*flags++) {
	  at->colour = given_colour;
	  total++;
	}
      }
    }
  }
  pop_atom_selection();

  if (message_mode)
    fprintf (stderr, "%i atoms selected for atomcolour\n", total);

  assert (count_atom_selections() == 0);
}


/*------------------------------------------------------------*/
void
set_atomcolour_bfactor (void)
{
  mol3d *mol;
  res3d *res;
  at3d *at;
  int *flags;
  int total = 0;
  double lower, upper, invdiff;

  assert (count_atom_selections() == 1);
  assert (dstack_size == 2);

  lower = dstack[0];
  upper = dstack[1];
  clear_dstack();

  if (upper <= lower) {
    yyerror ("invalid b-factor range values");
    return;
  }

  invdiff = 1.0 / (upper - lower);

  flags = current_atom_sel->flags;
  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) {
	if (*flags++) {
	  if (at->bfactor <= lower) {
	    at->colour = ramp_from_colour;
	  } else if (at->bfactor >= upper) {
	    at->colour = ramp_to_colour;
	  } else {
	    ramp_colour (&(at->colour), (at->bfactor - lower) * invdiff);
	  }
	  total++;
	}
      }
    }
  }
  pop_atom_selection();

  if (message_mode)
    fprintf (stderr, "%i atoms selected for atomcolour b-factor\n", total);

  assert (count_atom_selections() == 0);
}


/*------------------------------------------------------------*/
void
set_atomradius (void)
{
  mol3d *mol;
  res3d *res;
  at3d *at;
  int *flags;
  double radius;
  int total = 0;

  assert (count_atom_selections() == 1);
  assert (dstack_size == 1);

  radius = dstack[0];
  clear_dstack();

  if (radius < 0.0) {
    yyerror ("invalid atomradius value");
    return;
  }

  flags = current_atom_sel->flags;
  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) {
	if (*flags++) {
	  at->radius = radius;
	  total++;
	}
      }
    }
  }
  pop_atom_selection();

  if (message_mode)
    fprintf (stderr, "%i atoms selected for atomradius\n", total);

  assert (count_atom_selections() == 0);
}


/*------------------------------------------------------------*/
void
set_bonddistance (void)
{
  assert (dstack_size == 1);

  if (dstack[0] <= 0.0) {
    yyerror ("invalid bonddistance value");
  } else {
    current_state->bonddistance = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_bondcross (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 0.0) {
    yyerror ("invalid bondcross value");
  } else if (dstack[0] >= 0.01) {
    current_state->bondcross = dstack[0];
  } else {
    current_state->bondcross = 0.0;
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_coilradius (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 0.0) {
    yyerror ("invalid coilradius value");
  } else {
    current_state->coilradius = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_colourparts (boolean on)
{
  current_state->colourparts = on;
}


/*------------------------------------------------------------*/
void
set_colourramphsb (boolean hsb)
{
  current_state->hsbramp = hsb;
}


/*------------------------------------------------------------*/
void
set_cylinderradius (void)
{
  assert (dstack_size == 1);

  if (dstack[0] <= 0.0) {
    yyerror ("invalid cylinderradius value");
  } else {
    current_state->cylinderradius = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_depthcue (void)
{
  assert (dstack_size == 1);

  if ((dstack[0] < 0.0) || (dstack[0] > 1.0)) {
    yyerror ("invalid depthcue value");
  } else {
    current_state->depthcue = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_emissivecolour (void)
{
  current_state->emissivecolour = given_colour;
}


/*------------------------------------------------------------*/
void
set_helixthickness (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 0.0) {
    yyerror ("invalid helixthickness value");
  } else {
    current_state->helixthickness = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_helixwidth (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 0.1) {
    yyerror ("invalid helixwidth value");
  } else {
    current_state->helixwidth = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_hsbrampreverse (boolean on)
{
  current_state->hsbrampreverse = on;
}


/*------------------------------------------------------------*/
void
set_labelbackground (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 0.0) {
    yyerror ("invalid labelbackground value");
  } else {
    current_state->labelbackground = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_labelcentre (boolean on)
{
  current_state->labelcentre = on;
}


/*------------------------------------------------------------*/
void
set_labelclip (boolean on)
{
  current_state->labelclip = on;
}


/*------------------------------------------------------------*/
void
set_labelmask (const char *str)
{
  int length, slot;
  int *mask;

  assert (str);
  assert (*str);

  length = strlen (str);
  if (current_state->labelmasklength < length) {
    if (current_state->labelmask) {
      current_state->labelmask = realloc (current_state->labelmask,
					  length * sizeof (int));
    } else {
      current_state->labelmask = malloc (length * sizeof (int));
    }
    for (slot = current_state->labelmasklength; slot < length; slot++) {
      current_state->labelmask[slot] = 0;
    }
    current_state->labelmasklength = length;
  }

  mask = current_state->labelmask;
  for (slot = 0; slot < length; slot++) {
    switch (str[slot]) {
    case ' ':
      mask[slot] = 3 * (mask[slot] / 3);
      break;
    case 'l':
      mask[slot] = 3 * (mask[slot] / 3) + 1;
      break;
    case 'u':
      mask[slot] = 3 * (mask[slot] / 3) + 2;
      break;
    case 'r':
      mask[slot] = (mask[slot] % 3);
      break;
    case 'g':
      mask[slot] = (mask[slot] % 3) + 3;
      break;
    default:
      yyerror ("invalid labelmask");
      return;
    }
  }
}


/*------------------------------------------------------------*/
void
set_labeloffset (void)
{
  assert (dstack_size == 3);

  v3_initialize (&(current_state->labeloffset), dstack[0], dstack[1], dstack[2]);
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_labelrotation (boolean on)
{
  current_state->labelrotation = on;
}


/*------------------------------------------------------------*/
void
set_labelsize (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 1.0) {
    yyerror ("invalid labelsize value");
  } else {
    current_state->labelsize = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_lightambientintensity (void)
{
  assert (dstack_size == 1);

  if ((dstack[0] < 0.0) || (dstack[0] > 1.0)) {
    yyerror ("invalid lightambientintensity value");
  } else {
    current_state->lightambientintensity = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_lightattenuation (void)
{
  assert (dstack_size == 3);

  if ((dstack[0] < 0.0) || (dstack[1] < 0.0) || (dstack[2] < 0.0)) {
    yyerror ("invalid lightattenuation value(s)");
  } else {
    current_state->lightattenuation.x = dstack[0];
    current_state->lightattenuation.y = dstack[1];
    current_state->lightattenuation.z = dstack[2];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_lightcolour (void)
{
  current_state->lightcolour = given_colour;
}


/*------------------------------------------------------------*/
void
set_lightintensity (void)
{
  assert (dstack_size == 1);

  if ((dstack[0] < 0.0) || (dstack[0] > 1.0)) {
    yyerror ("invalid lightintensity value");
  } else {
    current_state->lightintensity = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_lightradius (void)
{
  assert (dstack_size == 1);

  if (dstack[0] <= 0.0) {
    yyerror ("invalid lightradius value");
  } else {
    current_state->lightradius = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_linecolour (void)
{
  current_state->linecolour = given_colour;
}


/*------------------------------------------------------------*/
void
set_linedash (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 0.0) {
    yyerror ("invalid linedash value");
  } else {
    current_state->linedash = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_linewidth (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 0.0) {
    yyerror ("invalid linewidth value");
  } else {
    current_state->linewidth = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_objecttransform (boolean on)
{
  current_state->objecttransform = on;
}


/*------------------------------------------------------------*/
void
set_planecolour (void)
{
  current_state->planecolour = given_colour;
}


/*------------------------------------------------------------*/
void
set_plane2colour (void)
{
  current_state->plane2colour = given_colour;
}


/*------------------------------------------------------------*/
void
set_residuecolour (void)
{
  mol3d *mol;
  res3d *res;
  int *flags;
  int total = 0;

  assert (count_residue_selections() == 1);

  flags = current_residue_sel->flags;
  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      if (*flags++) {
	res->colour = given_colour;
	total++;
      }
    }
  }
  pop_residue_selection();

  if (message_mode)
    fprintf (stderr, "%i residues selected for residuecolour\n", total);

  assert (count_residue_selections() == 0);
}


/*------------------------------------------------------------*/
void
set_residuecolour_bfactor (void)
{
  mol3d *mol;
  res3d *res;
  int *flags;
  int total = 0;
  at3d *at;
  double lower, upper, invdiff, bfactor, sum;

  assert (count_residue_selections() == 1);
  assert (dstack_size == 2);

  lower = dstack[0];
  upper = dstack[1];
  clear_dstack();

  if (upper <= lower) {
    yyerror ("invalid b-factor range values");
    return;
  }

  invdiff = 1.0 / (upper - lower);

  flags = current_residue_sel->flags;
  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      if (*flags++) {
	if (res->code != 'X') {
	  at = at3d_lookup (res, "CA");
	} else {
	  at = NULL;
	}

	if (at) {
	  bfactor = at->bfactor;
	} else {
	  bfactor = 0.0;
	  sum = 0.0;
	  for (at = res->first; at; at = at->next) {
	    bfactor += at->bfactor;
	    sum++;
	  }
	  if (sum != 0.0) bfactor /= sum;
	}

	if (bfactor <= lower) {
	  res->colour = ramp_from_colour;
	} else if (bfactor >= upper) {
	  res->colour = ramp_to_colour;
	} else {
	  ramp_colour (&(res->colour), (bfactor - lower) * invdiff);
	}
	total++;
      }
    }
  }
  pop_residue_selection();

  if (message_mode)
    fprintf (stderr, "%i residues selected for residuecolour b-factor\n", total);

  assert (count_residue_selections() == 0);
}


/*------------------------------------------------------------*/
void
set_residuecolour_seq (void)
{
  mol3d *mol;
  res3d *res;
  int *flags;
  int total = 0;
  double sum, f;

  assert (count_residue_selections() == 1);

  sum = (double) select_residue_count() - 1.0;
  flags = current_residue_sel->flags;
  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      if (*flags++) {
	f = ((double) total++) / sum;
	ramp_colour (&(res->colour), (f <= 1.0 ? f : 1.0));
      }
    }
  }
  pop_residue_selection();

  if (message_mode)
    fprintf (stderr, "%i residues selected for residuecolour sequence\n", total);

  assert (count_residue_selections() == 0);
}


/*------------------------------------------------------------*/
void
set_regularexpression (boolean on)
{
  current_state->regularexpression = on;
}


/*------------------------------------------------------------*/
void
set_segments (void)
{
  assert (dstack_size == 1);

  if (ival < 2) {
    yyerror ("invalid segments value");
  } else {
    current_state->segments = ival;
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_segmentsize (void)
{
  assert (dstack_size == 1);

  if (dstack[0] <= 0.0) {
    yyerror ("invalid segmentsize value");
  } else {
    current_state->segmentsize = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_shading (void)
{
  assert (dstack_size == 1);

  if ((dstack[0] < 0.0) || (dstack[0] > 1.0)) {
    yyerror ("invalid shading value");
  } else {
    current_state->shading = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_shadingexponent (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 0.0) {
    yyerror ("invalid shadingexponent value");
  } else {
    current_state->shadingexponent = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_shininess (void)
{
  assert (dstack_size == 1);

  if ((dstack[0] < 0.0) || (dstack[0] > 1.0)) {
    yyerror ("invalid shininess value");
  } else {
    current_state->shininess = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_smoothsteps (void)
{
  assert (dstack_size == 1);

  if (ival < 1) {
    yyerror ("invalid smoothsteps value");
  } else {
    current_state->smoothsteps = ival;
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_specularcolour (void)
{
  current_state->specularcolour = given_colour;
}


/*------------------------------------------------------------*/
void
set_splinefactor (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 0.01) {
    yyerror ("invalid splinefactor value");
  } else {
    current_state->splinefactor = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_stickradius (void)
{
  assert (dstack_size == 1);

  if (dstack[0] <= 0.0) {
    yyerror ("invalid stickradius value");
  } else {
    current_state->stickradius = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_sticktaper (void)
{
  assert (dstack_size == 1);

  if ((dstack[0] < 0.0) || (dstack[0] > 1.0)) {
    yyerror ("invalid sticktaper value");
  } else {
    current_state->sticktaper = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_strandthickness (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 0.0) {
    yyerror ("invalid strandthickness value");
  } else {
    current_state->strandthickness = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_strandwidth (void)
{
  assert (dstack_size == 1);

  if (dstack[0] < 0.02) {
    yyerror ("invalid strandwidth value");
  } else {
    current_state->strandwidth = dstack[0];
  }
  clear_dstack();
}


/*------------------------------------------------------------*/
void
set_transparency (void)
{
  assert (dstack_size == 1);

  if ((dstack[0] < 0.0) || (dstack[0] > 1.0)) {
    yyerror ("invalid transparency value");
  } else {
    current_state->transparency = dstack[0];
  }
  clear_dstack();
}
