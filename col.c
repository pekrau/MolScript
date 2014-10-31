/* col.c

   MolScript v2.1.2

   Colour routines.

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
    29-Jan-1997  use clib colour
*/

#include <assert.h>

#include "clib/colour.h"

#include "col.h"
#include "global.h"
#include "lex.h"
#include "state.h"


/*------------------------------------------------------------*/
colour given_colour;
colour white_colour = { COLOUR_GREY, 1.0, 0.0, 0.0 };
colour black_colour = { COLOUR_GREY, 0.0, 0.0, 0.0 };
colour grey_colour  = { COLOUR_GREY, 0.5, 0.0, 0.0 };
colour grey02_colour  = { COLOUR_GREY, 0.2, 0.0, 0.0 };
colour red_colour   = { COLOUR_RGB,  1.0, 0.0, 0.0 };
colour blue_colour  = { COLOUR_RGB,  0.0, 0.0, 1.0 };
colour ramp_from_colour, ramp_to_colour;


/*------------------------------------------------------------*/
void
constant_colours_to_rgb (void)
{
  colour_to_rgb (&white_colour);
  colour_to_rgb (&black_colour);
  colour_to_rgb (&grey_colour);
  colour_to_rgb (&grey02_colour);
}


/*------------------------------------------------------------*/
int
invalid_colour (colour *c)
{
  assert (c);

  if (c->x < 0.0 || c->x > 1.0) {
    if (c->spec == COLOUR_GREY) {
      yyerror ("invalid grey colour value");
      return TRUE;
    } else {
      yyerror ("invalid first component of colour value");
      return TRUE;
    }
  }

  if (c->y < 0.0 || c->y > 1.0) {
    yyerror ("invalid second component of colour value");
    return TRUE;
  }

  if (c->z < 0.0 || c->z > 1.0)  {
    yyerror ("invalid third component of colour value");
    return TRUE;
  }

  return FALSE;
}


/*------------------------------------------------------------*/
void
set_rgb (void)
{
  assert (dstack_size >= 3);

  colour_set_rgb (&given_colour,
		  dstack [dstack_size - 3],
		  dstack [dstack_size - 2],
		  dstack [dstack_size - 1]);
  pop_dstack (3);

  if (invalid_colour (&given_colour)) return;

  assert (colour_valid_state (&given_colour));
}


/*------------------------------------------------------------*/
void
set_hsb (void)
{
  assert (dstack_size >= 3);

  colour_set_hsb (&given_colour,
		  dstack [dstack_size - 3],
		  dstack [dstack_size - 2],
		  dstack [dstack_size - 1]);
  pop_dstack (3);

  if (invalid_colour (&given_colour)) return;

  assert (colour_valid_state (&given_colour));
}


/*------------------------------------------------------------*/
void
set_grey (void)
{
  assert (dstack_size >= 1);

  colour_set_grey (&given_colour, dstack [dstack_size - 1]);
  pop_dstack (1);

  if (invalid_colour (&given_colour)) return;

  assert (colour_valid_state (&given_colour));
}


/*------------------------------------------------------------*/
void
set_colour (const char *name)
{
  assert (name);

  if (! colour_set_name (&given_colour, name)) {
    yyerror ("unknown colour name");
    return;
  }

  assert (colour_valid_state (&given_colour));
}


/*------------------------------------------------------------*/
static double hsb_ramp_length;
static double hsb_ramp_breakpoint;


/*------------------------------------------------------------*/
void
set_colour_ramp (colour *ramp_to)
{
  assert (ramp_to);

  ramp_to_colour = *ramp_to;

  if (current_state->hsbramp) {

    colour_to_hsb (&ramp_from_colour);
    colour_to_hsb (&ramp_to_colour);

    if (current_state->hsbrampreverse) {
      if (ramp_from_colour.x < ramp_to_colour.x) {
	hsb_ramp_length = - (ramp_from_colour.x + (1.0 - ramp_to_colour.x));
	hsb_ramp_breakpoint = ramp_from_colour.x / (- hsb_ramp_length);
      } else {
	hsb_ramp_length = ramp_to_colour.x - ramp_from_colour.x;
	hsb_ramp_breakpoint = 1.0;
      }

    } else {
      if (ramp_from_colour.x > ramp_to_colour.x) {
	hsb_ramp_length = (1.0 - ramp_from_colour.x) + ramp_to_colour.x;
	hsb_ramp_breakpoint = (1.0 - ramp_from_colour.x) / hsb_ramp_length;
      } else {
	hsb_ramp_length = ramp_to_colour.x - ramp_from_colour.x;
	hsb_ramp_breakpoint = 1.0;
      }
    }

  } else {
    colour_to_rgb (&ramp_from_colour);
    colour_to_rgb (&ramp_to_colour);
  }

  assert (colour_valid_state (&ramp_from_colour));
  assert (colour_valid_state (&ramp_to_colour));
}


/*------------------------------------------------------------*/
void
set_rainbow_ramp (void)
{
  colour tmp_red_colour = { COLOUR_RGB, 1.0, 0.0, 0.0 };

  current_state->hsbramp = TRUE;
  current_state->hsbrampreverse = TRUE;

  ramp_from_colour = blue_colour;
  set_colour_ramp (&tmp_red_colour);
}


/*------------------------------------------------------------*/
void
ramp_colour (colour *dest, double f)
{
  assert (dest);
  assert (f >= 0.0);
  assert (f <= 1.0);

  if (current_state->hsbramp) {
    dest->spec = COLOUR_HSB;
    if (f <= hsb_ramp_breakpoint) {
      dest->x = ramp_from_colour.x + f * hsb_ramp_length;
    } else if (hsb_ramp_length < 0.0) {
      dest->x = 1.0 + (f - hsb_ramp_breakpoint) * hsb_ramp_length;
    } else {
      dest->x = (f - hsb_ramp_breakpoint) * hsb_ramp_length;
    }
    dest->y = ramp_from_colour.y + f * (ramp_to_colour.y - ramp_from_colour.y);
    dest->z = ramp_from_colour.z + f * (ramp_to_colour.z - ramp_from_colour.z);

  } else {
    dest->spec = COLOUR_RGB;
    dest->x = ramp_from_colour.x + f * (ramp_to_colour.x - ramp_from_colour.x);
    dest->y = ramp_from_colour.y + f * (ramp_to_colour.y - ramp_from_colour.y);
    dest->z = ramp_from_colour.z + f * (ramp_to_colour.z - ramp_from_colour.z);
  }

  assert (colour_valid_state (dest));
}
