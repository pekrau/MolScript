#ifndef COLOUR_H
#define COLOUR_H 1

#include <boolean.h>

enum colour_spec_codes { COLOUR_RGB = 0, COLOUR_HSB, COLOUR_GREY };

typedef struct {
  int spec;
  double x, y ,z;
} colour;

int
colour_names_count (void);

void
colour_name (char *name, colour *c, int number);

boolean
colour_set_name (colour *c, const char *name);

void
colour_set_rgb (colour *c, double r, double g, double b);

void
colour_set_hsb (colour *c, double h, double s, double b);

void
colour_set_grey (colour *c, double g);

void
colour_to_rgb (colour *c);

void
colour_to_hsb (colour *c);

void
colour_to_grey (colour *c);

void
colour_copy_to_rgb (colour *dest, colour *src);

void
colour_copy_to_hsb (colour *dest, colour *src);

void
colour_copy_to_grey (colour *dest, colour *src);

colour *
colour_clone (colour *c);

boolean
colour_approx_equal (const colour *c1, const colour *c2, double maxdiff);

boolean
colour_unequal (const colour *c1, const colour *c2);

void
colour_blend (colour *c, double fraction, colour *other);

void
colour_darker (colour *dest, double fraction, colour *src);

boolean
colour_valid_state (const colour *c);

#endif
