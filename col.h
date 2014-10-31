/* col.h

   MolScript v2.1.2

   Colour routine definitions.

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
    29-Jan-1997  use clib colour
*/

#ifndef COL_H
#define COL_H 1

#include "clib/colour.h"

extern colour given_colour;
extern colour white_colour, black_colour, grey_colour, grey02_colour,
              red_colour, blue_colour;
extern colour ramp_from_colour, ramp_to_colour;

void constant_colours_to_rgb (void);
int invalid_colour (colour *c);
void set_rgb (void);
void set_hsb (void);
void set_grey (void);
void set_colour (const char *name);

void set_colour_ramp (colour *ramp_to);
void set_rainbow_ramp (void);
void ramp_colour (colour *dest, double f);

#endif
