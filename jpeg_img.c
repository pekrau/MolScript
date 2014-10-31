/* jpeg_img.c

   MolScript v2.1.2

   JPEG image file.

   This implementation is based on the Independent JPEG Group's (IJG)
   JPEG library (release 6a). It relies on the 'image.c' code, and
   therefore implicitly on OpenGL and GLX.

   Copyright (C) 1997-1998 Per Kraulis
     9-Sep-1997  started
    11-Sep-1997  working
*/

#include <assert.h>
#include <stdlib.h>

#include <GL/gl.h>

#include "jpeg_img.h"
#include "global.h"
#include "graphics.h"
#include "image.h"
#include "opengl.h"

/* Must be defined at this position. */
#define HAVE_BOOLEAN
#include <jpeglib.h>


/*============================================================*/
static int quality = 90;
static struct jpeg_compress_struct cinfo;
static struct jpeg_error_mgr jerr;


/*------------------------------------------------------------*/
void
jpgi_set (void)
{
  ogl_set();

  output_first_plot = jpgi_first_plot;
  output_finish_output = jpgi_finish_output;
  output_start_plot = ogl_start_plot_general;

  output_pickable = NULL;

  output_mode = JPEG_MODE;
}


/*------------------------------------------------------------*/
void
jpgi_first_plot (void)
{
  image_first_plot();
  set_outfile ("wb");

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);
  jpeg_stdio_dest (&cinfo, outfile);

  cinfo.image_width = output_width;
  cinfo.image_height = output_height;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, quality, TRUE);
}


/*------------------------------------------------------------*/
void
jpgi_finish_output (void)
{
  int row;
  unsigned char *buffer;

  image_render();

  jpeg_start_compress (&cinfo, TRUE);
  buffer = malloc (output_width * 3 * sizeof (unsigned char));

  for (row = output_height - 1; row >= 0; row--) {
    glReadPixels (0, row, output_width, 1, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    jpeg_write_scanlines (&cinfo, &buffer, 1);
  }

  free (buffer);

  jpeg_finish_compress (&cinfo);
  jpeg_destroy_compress (&cinfo);

  image_close();
}


/*------------------------------------------------------------*/
void
jpgi_set_quality (int new)
{
  assert (new > 0);
  assert (new <= 100);

  quality = new;
}
