/*
   RGB, HSB and greyscale colour conversion and handling.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
    29-Jan-1997  split out of MolScript
    20-Dec-1997  fixed bug in colour_valid_state
    13-Mar-1998  added 'set' and 'copy_to' procedures

    to do:
    - colour_blend is incorrect for HSB
    - apparent linear hue function
*/

#include "colour.h"

/* public ==========
#include <boolean.h>

enum colour_spec_codes { COLOUR_RGB = 0, COLOUR_HSB, COLOUR_GREY };

typedef struct {
  int spec;
  double x, y ,z;
} colour;
========== public */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
  char name[21];
  int spec;
  double x, y, z;
} named_colour;

static named_colour named_colours[] = {
  { "aliceblue", COLOUR_RGB, 0.941176, 0.972549, 1 },
  { "antiquewhite", COLOUR_RGB, 0.980392, 0.921569, 0.843137 },
  { "aquamarine", COLOUR_RGB, 0.498039, 1, 0.831373 },
  { "azure", COLOUR_RGB, 0.941176, 1, 1 },
  { "beige", COLOUR_RGB, 0.960784, 0.960784, 0.862745 },
  { "bisque", COLOUR_RGB, 1, 0.894118, 0.768627 },
  { "black", COLOUR_GREY, 0, 0, 0 },
  { "blanchedalmond", COLOUR_RGB, 1, 0.921569, 0.803922 },
  { "blue", COLOUR_RGB, 0, 0, 1 },
  { "blueviolet", COLOUR_RGB, 0.541176, 0.168627, 0.886275 },
  { "brown", COLOUR_RGB, 0.647059, 0.164706, 0.164706 },
  { "burlywood", COLOUR_RGB, 0.870588, 0.721569, 0.529412 },
  { "cadetblue", COLOUR_RGB, 0.372549, 0.619608, 0.627451 },
  { "chartreuse", COLOUR_RGB, 0.498039, 1, 0 },
  { "chocolate", COLOUR_RGB, 0.823529, 0.411765, 0.117647 },
  { "coral", COLOUR_RGB, 1, 0.498039, 0.313725 },
  { "cornflowerblue", COLOUR_RGB, 0.392157, 0.584314, 0.929412 },
  { "cornsilk", COLOUR_RGB, 1, 0.972549, 0.862745 },
  { "crimson", COLOUR_RGB, 0.862745, 0.0784314, 0.235294 },
  { "cyan", COLOUR_RGB, 0, 1, 1 },
  { "darkblue", COLOUR_RGB, 0, 0, 0.545098 },
  { "darkcyan", COLOUR_RGB, 0, 0.545098, 0.545098 },
  { "darkgoldenrod", COLOUR_RGB, 0.721569, 0.52549, 0.0431373 },
  { "darkgray", COLOUR_RGB, 0.662745, 0.662745, 0.662745 },
  { "darkgreen", COLOUR_RGB, 0, 0.392157, 0 },
  { "darkgrey", COLOUR_RGB, 0.662745, 0.662745, 0.662745 },
  { "darkkhaki", COLOUR_RGB, 0.741176, 0.717647, 0.419608 },
  { "darkmagenta", COLOUR_RGB, 0.545098, 0, 0.545098 },
  { "darkolivegreen", COLOUR_RGB, 0.333333, 0.419608, 0.184314 },
  { "darkorange", COLOUR_RGB, 1, 0.54902, 0 },
  { "darkorchid", COLOUR_RGB, 0.6, 0.196078, 0.8 },
  { "darkred", COLOUR_RGB, 0.545098, 0, 0 },
  { "darksalmon", COLOUR_RGB, 0.913725, 0.588235, 0.478431 },
  { "darkseagreen", COLOUR_RGB, 0.560784, 0.737255, 0.560784 },
  { "darkslateblue", COLOUR_RGB, 0.282353, 0.239216, 0.545098 },
  { "darkslategray", COLOUR_RGB, 0.184314, 0.309804, 0.309804 },
  { "darkslategrey", COLOUR_RGB, 0.184314, 0.309804, 0.309804 },
  { "darkturquoise", COLOUR_RGB, 0, 0.807843, 0.819608 },
  { "darkviolet", COLOUR_RGB, 0.580392, 0, 0.827451 },
  { "deeppink", COLOUR_RGB, 1, 0.0784314, 0.576471 },
  { "deepskyblue", COLOUR_RGB, 0, 0.74902, 1 },
  { "dimgray", COLOUR_RGB, 0.411765, 0.411765, 0.411765 },
  { "dimgrey", COLOUR_RGB, 0.411765, 0.411765, 0.411765 },
  { "dodgerblue", COLOUR_RGB, 0.117647, 0.564706, 1 },
  { "firebrick", COLOUR_RGB, 0.698039, 0.133333, 0.133333 },
  { "floralwhite", COLOUR_RGB, 1, 0.980392, 0.941176 },
  { "forestgreen", COLOUR_RGB, 0.133333, 0.545098, 0.133333 },
  { "gainsboro", COLOUR_RGB, 0.862745, 0.862745, 0.862745 },
  { "ghostwhite", COLOUR_RGB, 0.972549, 0.972549, 1 },
  { "gold", COLOUR_RGB, 1, 0.843137, 0 },
  { "goldenrod", COLOUR_RGB, 0.854902, 0.647059, 0.12549 },
  { "gray", COLOUR_RGB, 0.745098, 0.745098, 0.745098 },
  { "green", COLOUR_RGB, 0, 1, 0 },
  { "greenyellow", COLOUR_RGB, 0.678431, 1, 0.184314 },
  { "grey", COLOUR_RGB, 0.745098, 0.745098, 0.745098 },
  { "honeydew", COLOUR_RGB, 0.941176, 1, 0.941176 },
  { "hotpink", COLOUR_RGB, 1, 0.411765, 0.705882 },
  { "indianred", COLOUR_RGB, 0.803922, 0.360784, 0.360784 },
  { "indigo", COLOUR_RGB, 0.294118, 0, 0.509804 },
  { "ivory", COLOUR_RGB, 1, 1, 0.941176 },
  { "khaki", COLOUR_RGB, 0.941176, 0.901961, 0.54902 },
  { "lavender", COLOUR_RGB, 0.901961, 0.901961, 0.980392 },
  { "lavenderblush", COLOUR_RGB, 1, 0.941176, 0.960784 },
  { "lawngreen", COLOUR_RGB, 0.486275, 0.988235, 0 },
  { "lemonchiffon", COLOUR_RGB, 1, 0.980392, 0.803922 },
  { "lightblue", COLOUR_RGB, 0.678431, 0.847059, 0.901961 },
  { "lightcoral", COLOUR_RGB, 0.941176, 0.501961, 0.501961 },
  { "lightcyan", COLOUR_RGB, 0.878431, 1, 1 },
  { "lightgoldenrod", COLOUR_RGB, 0.933333, 0.866667, 0.509804 },
  { "lightgoldenrodyellow", COLOUR_RGB, 0.980392, 0.980392, 0.823529 },
  { "lightgray", COLOUR_RGB, 0.827451, 0.827451, 0.827451 },
  { "lightgreen", COLOUR_RGB, 0.564706, 0.933333, 0.564706 },
  { "lightgrey", COLOUR_RGB, 0.827451, 0.827451, 0.827451 },
  { "lightpink", COLOUR_RGB, 1, 0.713725, 0.756863 },
  { "lightsalmon", COLOUR_RGB, 1, 0.627451, 0.478431 },
  { "lightseagreen", COLOUR_RGB, 0.12549, 0.698039, 0.666667 },
  { "lightskyblue", COLOUR_RGB, 0.529412, 0.807843, 0.980392 },
  { "lightslateblue", COLOUR_RGB, 0.517647, 0.439216, 1 },
  { "lightslategray", COLOUR_RGB, 0.466667, 0.533333, 0.6 },
  { "lightslategrey", COLOUR_RGB, 0.466667, 0.533333, 0.6 },
  { "lightsteelblue", COLOUR_RGB, 0.690196, 0.768627, 0.870588 },
  { "lightyellow", COLOUR_RGB, 1, 1, 0.878431 },
  { "limegreen", COLOUR_RGB, 0.196078, 0.803922, 0.196078 },
  { "linen", COLOUR_RGB, 0.980392, 0.941176, 0.901961 },
  { "magenta", COLOUR_RGB, 1, 0, 1 },
  { "maroon", COLOUR_RGB, 0.690196, 0.188235, 0.376471 },
  { "mediumaquamarine", COLOUR_RGB, 0.4, 0.803922, 0.666667 },
  { "mediumblue", COLOUR_RGB, 0, 0, 0.803922 },
  { "mediumorchid", COLOUR_RGB, 0.729412, 0.333333, 0.827451 },
  { "mediumpurple", COLOUR_RGB, 0.576471, 0.439216, 0.858824 },
  { "mediumseagreen", COLOUR_RGB, 0.235294, 0.701961, 0.443137 },
  { "mediumslateblue", COLOUR_RGB, 0.482353, 0.407843, 0.933333 },
  { "mediumspringgreen", COLOUR_RGB, 0, 0.980392, 0.603922 },
  { "mediumturquoise", COLOUR_RGB, 0.282353, 0.819608, 0.8 },
  { "mediumvioletred", COLOUR_RGB, 0.780392, 0.0823529, 0.521569 },
  { "midnightblue", COLOUR_RGB, 0.0980392, 0.0980392, 0.439216 },
  { "mintcream", COLOUR_RGB, 0.960784, 1, 0.980392 },
  { "mistyrose", COLOUR_RGB, 1, 0.894118, 0.882353 },
  { "moccasin", COLOUR_RGB, 1, 0.894118, 0.709804 },
  { "navajowhite", COLOUR_RGB, 1, 0.870588, 0.678431 },
  { "navy", COLOUR_RGB, 0, 0, 0.501961 },
  { "navyblue", COLOUR_RGB, 0, 0, 0.501961 },
  { "oldlace", COLOUR_RGB, 0.992157, 0.960784, 0.901961 },
  { "olivedrab", COLOUR_RGB, 0.419608, 0.556863, 0.137255 },
  { "orange", COLOUR_RGB, 1, 0.647059, 0 },
  { "orangered", COLOUR_RGB, 1, 0.270588, 0 },
  { "orchid", COLOUR_RGB, 0.854902, 0.439216, 0.839216 },
  { "palegoldenrod", COLOUR_RGB, 0.933333, 0.909804, 0.666667 },
  { "palegreen", COLOUR_RGB, 0.596078, 0.984314, 0.596078 },
  { "paleturquoise", COLOUR_RGB, 0.686275, 0.933333, 0.933333 },
  { "palevioletred", COLOUR_RGB, 0.858824, 0.439216, 0.576471 },
  { "papayawhip", COLOUR_RGB, 1, 0.937255, 0.835294 },
  { "peachpuff", COLOUR_RGB, 1, 0.854902, 0.72549 },
  { "peru", COLOUR_RGB, 0.803922, 0.521569, 0.247059 },
  { "pink", COLOUR_RGB, 1, 0.752941, 0.796078 },
  { "plum", COLOUR_RGB, 0.866667, 0.627451, 0.866667 },
  { "powderblue", COLOUR_RGB, 0.690196, 0.878431, 0.901961 },
  { "purple", COLOUR_RGB, 0.627451, 0.12549, 0.941176 },
  { "red", COLOUR_RGB, 1, 0, 0 },
  { "rosybrown", COLOUR_RGB, 0.737255, 0.560784, 0.560784 },
  { "royalblue", COLOUR_RGB, 0.254902, 0.411765, 0.882353 },
  { "saddlebrown", COLOUR_RGB, 0.545098, 0.270588, 0.0745098 },
  { "salmon", COLOUR_RGB, 0.980392, 0.501961, 0.447059 },
  { "sandybrown", COLOUR_RGB, 0.956863, 0.643137, 0.376471 },
  { "seagreen", COLOUR_RGB, 0.180392, 0.545098, 0.341176 },
  { "seashell", COLOUR_RGB, 1, 0.960784, 0.933333 },
  { "sgibeet", COLOUR_RGB, 0.556863, 0.219608, 0.556863 },
  { "sgibrightgray", COLOUR_RGB, 0.772549, 0.756863, 0.666667 },
  { "sgibrightgrey", COLOUR_RGB, 0.772549, 0.756863, 0.666667 },
  { "sgichartreuse", COLOUR_RGB, 0.443137, 0.776471, 0.443137 },
  { "sgidarkgray", COLOUR_RGB, 0.333333, 0.333333, 0.333333 },
  { "sgidarkgrey", COLOUR_RGB, 0.333333, 0.333333, 0.333333 },
  { "sgilightblue", COLOUR_RGB, 0.490196, 0.619608, 0.752941 },
  { "sgilightgray", COLOUR_RGB, 0.666667, 0.666667, 0.666667 },
  { "sgilightgrey", COLOUR_RGB, 0.666667, 0.666667, 0.666667 },
  { "sgimediumgray", COLOUR_RGB, 0.517647, 0.517647, 0.517647 },
  { "sgimediumgrey", COLOUR_RGB, 0.517647, 0.517647, 0.517647 },
  { "sgiolivedrab", COLOUR_RGB, 0.556863, 0.556863, 0.219608 },
  { "sgisalmon", COLOUR_RGB, 0.776471, 0.443137, 0.443137 },
  { "sgislateblue", COLOUR_RGB, 0.443137, 0.443137, 0.776471 },
  { "sgiteal", COLOUR_RGB, 0.219608, 0.556863, 0.556863 },
  { "sgiverydarkgray", COLOUR_RGB, 0.156863, 0.156863, 0.156863 },
  { "sgiverydarkgrey", COLOUR_RGB, 0.156863, 0.156863, 0.156863 },
  { "sgiverylightgray", COLOUR_RGB, 0.839216, 0.839216, 0.839216 },
  { "sgiverylightgrey", COLOUR_RGB, 0.839216, 0.839216, 0.839216 },
  { "sienna", COLOUR_RGB, 0.627451, 0.321569, 0.176471 },
  { "skyblue", COLOUR_RGB, 0.529412, 0.807843, 0.921569 },
  { "slateblue", COLOUR_RGB, 0.415686, 0.352941, 0.803922 },
  { "slategray", COLOUR_RGB, 0.439216, 0.501961, 0.564706 },
  { "slategrey", COLOUR_RGB, 0.439216, 0.501961, 0.564706 },
  { "snow", COLOUR_RGB, 1, 0.980392, 0.980392 },
  { "springgreen", COLOUR_RGB, 0, 1, 0.498039 },
  { "steelblue", COLOUR_RGB, 0.27451, 0.509804, 0.705882 },
  { "tan", COLOUR_RGB, 0.823529, 0.705882, 0.54902 },
  { "thistle", COLOUR_RGB, 0.847059, 0.74902, 0.847059 },
  { "tomato", COLOUR_RGB, 1, 0.388235, 0.278431 },
  { "turquoise", COLOUR_RGB, 0.25098, 0.878431, 0.815686 },
  { "violet", COLOUR_RGB, 0.933333, 0.509804, 0.933333 },
  { "violetred", COLOUR_RGB, 0.815686, 0.12549, 0.564706 },
  { "wheat", COLOUR_RGB, 0.960784, 0.870588, 0.701961 },
  { "white", COLOUR_GREY, 1, 0, 0 },
  { "whitesmoke", COLOUR_RGB, 0.960784, 0.960784, 0.960784 },
  { "yellow", COLOUR_RGB, 1, 1, 0 },
  { "yellowgreen", COLOUR_RGB, 0.603922, 0.803922, 0.196078 },
};

static const int named_colour_count = 164;


/*------------------------------------------------------------*/
static int
cmp (const void *keyval, const void *datum)
{
  return strcmp ((char *) keyval, ((named_colour *) datum)->name);
}


/*------------------------------------------------------------*/
int
colour_names_count (void)
     /*
       Return the number of defined colour names.
     */
{
  return named_colour_count;
}


/*------------------------------------------------------------*/
void
colour_name (char *name, colour *c, int number)
     /*
       Return the name and definition of a named colour by its number
       in the internal table.
       The name argument must have space for at least 20 characters.
     */
{
  /* pre */
  assert (name);
  assert (c);
  assert (number >= 0);
  assert (number < colour_names_count());

  strcpy (name, named_colours[number].name);
  c->spec = named_colours[number].spec;
  c->x = named_colours[number].x;
  c->y = named_colours[number].y;
  c->z = named_colours[number].z;
}


/*------------------------------------------------------------*/
boolean
colour_set_name (colour *c, const char *name)
     /*
       Set the colour given the name. The name must be in lower case,
       and may not contain any whitespace. All returned colours
       are specified in COLOUR_RGB, except for white and black,
       which are specified in COLOUR_GREY.
       Return TRUE if the name is found in the list, otherwise FALSE.
     */
{
  named_colour *col;

  /* pre */
  assert (c);
  assert (name);
  assert (*name);

  col = bsearch (name, named_colours, named_colour_count,
		 sizeof (named_colour), cmp);
  if (col) {
    c->spec = col->spec;
    c->x = col->x;
    c->y = col->y;
    c->z = col->z;
    return TRUE;
  } else {
    return FALSE;
  }
}


/*------------------------------------------------------------*/
void
colour_set_rgb (colour *c, double r, double g, double b)
{
  /* pre */
  assert (c);
  assert (r >= 0.0);
  assert (r <= 1.0);
  assert (g >= 0.0);
  assert (g <= 1.0);
  assert (b >= 0.0);
  assert (b <= 1.0);

  c->spec = COLOUR_RGB;
  c->x = r;
  c->y = g;
  c->z = b;

  assert (colour_valid_state (c));
}


/*------------------------------------------------------------*/
void
colour_set_hsb (colour *c, double h, double s, double b)
{
  /* pre */
  assert (c);
  assert (h >= 0.0);
  assert (h <= 1.0);
  assert (s >= 0.0);
  assert (s <= 1.0);
  assert (b >= 0.0);
  assert (b <= 1.0);

  c->spec = COLOUR_HSB;
  c->x = h;
  c->y = s;
  c->z = b;

  assert (colour_valid_state (c));
}


/*------------------------------------------------------------*/
void
colour_set_grey (colour *c, double g)
{
  /* pre */
  assert (c);
  assert (g >= 0.0);
  assert (g <= 1.0);

  c->spec = COLOUR_GREY;
  c->x = g;

  assert (colour_valid_state (c));
}


/*------------------------------------------------------------*/
static void
rgb_to_hsb (colour *c)
{
  double max, min;
  colour hsb;

  assert (c);
  assert (c->spec == COLOUR_RGB);
  assert (colour_valid_state (c));

  min = (c->x < c->y) ? c->x : c->y;
  if (c->z < min) min = c->z;
  max = (c->x > c->y) ? c->x : c->y;
  if (c->z > max) max = c->z;

  hsb.spec = COLOUR_HSB;
  hsb.z = max;

  if (max != 0.0) {
    hsb.y = (max - min) / max;
  } else {
    hsb.y = 0.0;
  }

  if (hsb.y == 0.0) {
    hsb.x = 0.0;

  } else {
    double delta = max - min;
    if (c->x == max) {
      hsb.x = (c->y - c->z) / delta;
    } else if (c->y == max) {
      hsb.x = 2.0 + (c->z - c->x) / delta;
    } else if (c->z == max) {
      hsb.x = 4.0 + (c->x - c->y) / delta;
    }
    hsb.x /= 6.0;
    if (hsb.x < 0.0) hsb.x += 1.0;
  }

  c->spec = hsb.spec;
  c->x = hsb.x;
  c->y = hsb.y;
  c->z = hsb.z;

  assert (colour_valid_state (c));
}


/*------------------------------------------------------------*/
static void
hsb_to_rgb (colour *c)
{
  assert (c);
  assert (c->spec == COLOUR_HSB);
  assert (colour_valid_state (c));

  if (c->y == 0.0) {
    c->x = c->z;
    c->y = c->z;

  } else {
    colour rgb;
    double i, f, p, q, t;

    if (c->x == 1.0) c->x = 0.0;
    c->x = 6.0 * c->x;
    i = floor (c->x);
    f = c->x - i;
    p = c->z * (1.0 - c->y);
    q = c->z * (1.0 - c->y * f);
    t = c->z * (1.0 - c->y * (1.0 - f));

    rgb.spec = COLOUR_RGB;
    switch ((int) i) {
    case 0:
      rgb.x = c->z;
      rgb.y = t;
      rgb.z = p;
      break;
    case 1:
      rgb.x = q;
      rgb.y = c->z;
      rgb.z = p;
      break;
    case 2:
      rgb.x = p;
      rgb.y = c->z;
      rgb.z = t;
      break;
    case 3:
      rgb.x = p;
      rgb.y = q;
      rgb.z = c->z;
      break;
    case 4:
      rgb.x = t;
      rgb.y = p;
      rgb.z = c->z;
      break;
    case 5:
      rgb.x = c->z;
      rgb.y = p;
      rgb.z = q;
      break;
    }

    c->spec = rgb.spec;
    c->x = rgb.x;
    c->y = rgb.y;
    c->z = rgb.z;
  }

  assert (colour_valid_state (c));
}


/*------------------------------------------------------------*/
void
colour_to_rgb (colour *c)
{
  /* pre */
  assert (c);
  assert (colour_valid_state (c));

  switch (c->spec) {
  case COLOUR_RGB:
    break;
  case COLOUR_HSB:
    hsb_to_rgb (c);
    break;
  case COLOUR_GREY:
    c->y = c->x;
    c->z = c->x;
    c->spec = COLOUR_RGB;
    break;
  }

  assert (colour_valid_state (c));
}


/*------------------------------------------------------------*/
void
colour_to_hsb (colour *c)
{
  /* pre */
  assert (c);
  assert (colour_valid_state (c));

  switch (c->spec) {
  case COLOUR_RGB:
    rgb_to_hsb (c);
    c->spec = COLOUR_HSB;
    break;
  case COLOUR_HSB:
    break;
  case COLOUR_GREY:
    c->z = c->x;
    c->y = 0.0;
    c->x = 0.0;
    c->spec = COLOUR_HSB;
    break;
  }

  assert (colour_valid_state (c));
}


/*------------------------------------------------------------*/
void
colour_to_grey (colour *c)
     /*
       Convert the colour to grey (with possible loss of information).
     */
{
  /* pre */
  assert (c);
  assert (colour_valid_state (c));

  switch (c->spec) {
  case COLOUR_RGB:
    rgb_to_hsb (c);
    colour_to_grey (c);
    break;
  case COLOUR_HSB:
    c->x = c->z;
    c->spec = COLOUR_GREY;
    break;
  case COLOUR_GREY:
    break;
  }

  assert (colour_valid_state (c));
}


/*------------------------------------------------------------*/
void
colour_copy_to_rgb (colour *dest, colour *src)
     /*
       Copy the colour 'src' to 'dest' and convert to RGB.
     */
{
  /* pre */
  assert (dest);
  assert (src);
  assert (colour_valid_state (src));

  dest->spec = src->spec;
  dest->x = src->x;
  dest->y = src->y;
  dest->z = src->z;
  colour_to_rgb (dest);

  assert (colour_valid_state (dest));
}


/*------------------------------------------------------------*/
void
colour_copy_to_hsb (colour *dest, colour *src)
     /*
       Copy the colour 'src' to 'dest' and convert to HSB.
     */
{
  /* pre */
  assert (dest);
  assert (src);
  assert (colour_valid_state (src));

  dest->spec = src->spec;
  dest->x = src->x;
  dest->y = src->y;
  dest->z = src->z;
  colour_to_hsb (dest);

  assert (colour_valid_state (dest));
}


/*------------------------------------------------------------*/
void
colour_copy_to_grey (colour *dest, colour *src)
     /*
       Copy the colour 'src' to 'dest' and convert to grey.
     */
{
  /* pre */
  assert (dest);
  assert (src);
  assert (colour_valid_state (src));

  dest->spec = src->spec;
  dest->x = src->x;
  dest->y = src->y;
  dest->z = src->z;
  colour_to_grey (dest);

  assert (colour_valid_state (dest));
}


/*------------------------------------------------------------*/
colour *
colour_clone (colour *c)
{
  colour *cc;

  /* pre */
  assert (c);

  cc = malloc (sizeof (colour));
  cc->spec = c->spec;
  cc->x = c->x;
  cc->y = c->y;
  cc->z = c->z;

  return cc;
}


/*------------------------------------------------------------*/
boolean
colour_approx_equal (const colour *c1, const colour *c2, double maxdiff)
     /*
       Are the two colours approximately equal? The colour specifications
       must be equal. The 'maxdiff' is the maximum allowed difference in
       component values.
     */
{
  /* pre */
  assert (c1);
  assert (c2);
  assert (maxdiff >= 0.0);

  if (c1->spec != c2->spec) return FALSE;
  if (fabs (c1->x - c2->x) > maxdiff) return FALSE;
  if (c1->spec != COLOUR_GREY) {
    if (fabs (c1->y - c2->y) > maxdiff) return FALSE;
    if (fabs (c1->z - c2->z) > maxdiff) return FALSE;
  }
  return TRUE;
}


/*------------------------------------------------------------*/
boolean
colour_unequal (const colour *c1, const colour *c2)
     /*
       Are the two colours unequal? Any difference in colour specification
       or components is tested.
     */
{
  /* pre */
  assert (c1);
  assert (c2);

  if (c1->spec != c2->spec) return TRUE;
  if (fabs (c1->x - c2->x) >= 0.0005) return TRUE;
  if (c1->spec != COLOUR_GREY) {
    if (fabs (c1->y - c2->y) >= 0.0005) return TRUE;
    if (fabs (c1->z - c2->z) >= 0.0005) return TRUE;
  }
  return FALSE;
}


/*------------------------------------------------------------*/
void
colour_blend (colour *c, double fraction, colour *other)
     /*
       Blend the 'other' colour into 'c' by the given fraction.
       The interpolation is currently done in RGB space.
       NOTE: this is incorrect for HSB colours.
     */
{
  colour col;

  /* pre */
  assert (c);
  assert (colour_valid_state (c));
  assert (other);
  assert (colour_valid_state (other));
  assert (fraction >= 0.0);
  assert (fraction <= 1.0);

  col.spec = other->spec;
  col.x = other->x;
  col.y = other->y;
  col.z = other->z;

  switch (c->spec) {
  case COLOUR_RGB:
    colour_to_rgb (&col);
    c->x = (1.0 - fraction) * c->x + fraction * col.x;
    c->y = (1.0 - fraction) * c->y + fraction * col.y;
    c->z = (1.0 - fraction) * c->z + fraction * col.z;
    break;
  case COLOUR_HSB:
    colour_to_rgb (c);
    colour_to_rgb (&col);
    c->x = (1.0 - fraction) * c->x + fraction * col.x;
    c->y = (1.0 - fraction) * c->y + fraction * col.y;
    c->z = (1.0 - fraction) * c->z + fraction * col.z;
    colour_to_hsb (c);
    break;
  case COLOUR_GREY:
    colour_to_grey (&col);
    c->x = (1.0 - fraction) * c->x + fraction * col.x;
    break;
  }

  assert (colour_valid_state (c));
}


/*------------------------------------------------------------*/
void
colour_darker (colour *dest, double fraction, colour *src)
     /*
       Compute a darker value from the given 'src' and 'fraction' and
       return in 'dest'.
     */
{
  /* pre */
  assert (dest);
  assert (fraction >= 0.0);
  assert (fraction <= 1.0);
  assert (src);
  assert (colour_valid_state (src));

  dest->spec = src->spec;

  switch (src->spec) {
  case COLOUR_RGB:
    dest->x = fraction * src->x;
    dest->y = fraction * src->y;
    dest->z = fraction * src->z;
    break;
  case COLOUR_HSB:
    dest->x = src->x;
    dest->y = src->y;
    dest->z = fraction * src->z;
    break;
  case COLOUR_GREY:
    dest->x = fraction * src->x;
    break;
  }

  assert (colour_valid_state (dest));
}


/*------------------------------------------------------------*/
boolean
colour_valid_state (const colour *c)
     /*
       Is the colour value in a valid state?
       The colour specification must be COLOUR_RGB, COLOUR_HSB or
       COLOUR_GREY, and the components must be in valid ranges.
     */
{
  /* pre */
  assert (c);

  if ((c->spec != COLOUR_RGB) &&
      (c->spec != COLOUR_HSB) &&
      (c->spec != COLOUR_GREY)) return FALSE;

  if ((c->x < 0.0) || (c->x > 1.0)) return FALSE;
  if (c->spec != COLOUR_GREY) {
    if ((c->y < 0.0) || (c->y > 1.0)) return FALSE;
    if ((c->z < 0.0) || (c->z > 1.0)) return FALSE;
  }

  return TRUE;
}
