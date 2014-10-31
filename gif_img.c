/* gif_img.c

   MolScript v2.1.2

   GIF image file.

   This implementation uses the gd 1.3 library by Thomas Boutell,
   http://www.boutell.com. The source code for this library does *not*
   use LZW compression, so there is no conflict with the (infamous)
   Unisys patent on the LZW algorithm. This implementation relies on
   the 'image.c' code, and therefore implicitly on OpenGL and GLX.

   Copyright (C) 1998 Per Kraulis
    29-Jul-1998  first attempts
*/

#include <assert.h>
#include <stdlib.h>

#include <GL/gl.h>

#include <gd.h>

#include "gif_img.h"
#include "global.h"
#include "graphics.h"
#include "image.h"
#include "opengl.h"


/*============================================================*/
static gdImagePtr image;


/*------------------------------------------------------------*/
void
gifi_set (void)
{
  ogl_set();

  output_first_plot = gifi_first_plot;
  output_finish_output = gifi_finish_output;
  output_start_plot = ogl_start_plot_general;

  output_pickable = NULL;

  output_mode = GIF_MODE;
}


/*------------------------------------------------------------*/
void
gifi_first_plot (void)
{
  int r, g, b, slot;

  image_first_plot();
  set_outfile ("wb");

  image = gdImageCreate (output_width, output_height);

  for (r = 0; r < 6; r++) {
    for (g = 0; g < 6; g++) {
      for (b = 0; b < 6; b++) {
	slot = gdImageColorAllocate (image, 51 * r, 51 * g, 51 * b);
	assert (slot >= 0);
      }
    }
  }
}


/*------------------------------------------------------------*/
void
gifi_finish_output (void)
{
  int row, col, value, r, g, b, error, right, leftbelow, below;
  unsigned char *buffer, *buf;
  int *rcurr, *gcurr, *bcurr, *rnext, *gnext, *bnext, *swap;

  image_render();

  buffer = malloc (output_width * 3 * sizeof (unsigned char));
  rcurr = malloc ((output_width + 1) * sizeof (int));
  gcurr = malloc ((output_width + 1) * sizeof (int));
  bcurr = malloc ((output_width + 1) * sizeof (int));
  rnext = calloc (output_width + 1, sizeof (int));
  gnext = calloc (output_width + 1, sizeof (int));
  bnext = calloc (output_width + 1, sizeof (int));

  for (row = 0; row < output_height; row++) {

    swap = rnext; rnext = rcurr; rcurr = swap;
    swap = gnext; gnext = gcurr; gcurr = swap;
    swap = bnext; bnext = bcurr; bcurr = swap;
    for (col = 0; col < output_width; col++) {
      rnext[col] = 0;
      gnext[col] = 0;
      bnext[col] = 0;
    }

    glReadPixels (0, output_height - row - 1, output_width, 1,
		  GL_RGB, GL_UNSIGNED_BYTE, buffer);
    buf = buffer;
    for (col = 0; col < output_width; col++) {
      rcurr[col] += *buf++;
      gcurr[col] += *buf++;
      bcurr[col] += *buf++;
    }

    for (col = 0; col < output_width; col++) { /* error diffusion */

      value = rcurr[col];
      for (r = 0; 51 * (r + 1) - 26 < value; r++);
      error = value - r * 51;
      right = (7 * error) / 16;
      leftbelow = (3 * error) / 16;
      below = (5 * error) / 16;
      rcurr[col+1] += right;
      if (col > 0) rnext[col-1] += leftbelow;
      rnext[col] += below;
      rnext[col+1] = error - right - leftbelow - below;

      value = gcurr[col];
      for (g = 0; 51 * (g + 1) - 26 < value; g++);
      error = value - g * 51;
      right = (7 * error) / 16;
      leftbelow = (3 * error) / 16;
      below = (5 * error) / 16;
      gcurr[col+1] += right;
      if (col > 0) gnext[col-1] += leftbelow;
      gnext[col] += below;
      gnext[col+1] = error - right - leftbelow - below;

      value = bcurr[col];
      for (b = 0; 51 * (b + 1) - 26 < value; b++);
      error = value - b * 51;
      right = (7 * error) / 16;
      leftbelow = (3 * error) / 16;
      below = (5 * error) / 16;
      bcurr[col+1] += right;
      if (col > 0) bnext[col-1] += leftbelow;
      bnext[col] += below;
      bnext[col+1] = error - right - leftbelow - below;

      gdImageSetPixel (image, col, row, ((r * 6) + g) * 6 + b);
    }
  }

  free (rnext);
  free (gnext);
  free (bnext);
  free (rcurr);
  free (gcurr);
  free (bcurr);
  free (buffer);

  gdImageGif (image, outfile);
  gdImageDestroy (image);

  image_close();
}
