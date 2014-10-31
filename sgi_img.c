/* sgi_img.c

   MolScript v2.1.2

   SGI image file.

   This implementation relies on the 'image.c' code, and therefore
   implicitly on OpenGL and GLX.

   Copyright (C) 1997-1998 Per Kraulis
     9-Sep-1997  started
    11-Sep-1997  working
    26-Sep-1997  use my own SGI image file interface
*/

#include <stdlib.h>

#include <GL/gl.h>

#include "clib/sgi_image.h"

#include "sgi_img.h"
#include "global.h"
#include "graphics.h"
#include "image.h"
#include "opengl.h"


/*============================================================*/
static sgi_image *image;


/*------------------------------------------------------------*/
void
sgii_set (void)
{
  ogl_set();

  output_first_plot = sgii_first_plot;
  output_finish_output = sgii_finish_output;
  output_start_plot = ogl_start_plot_general;

  output_pickable = NULL;

  output_mode = SGI_MODE;
}


/*------------------------------------------------------------*/
void
sgii_first_plot (void)
{
  image_first_plot();
  set_outfile ("wb");

  image = sgiimg_create();
  sgiimg_set_size (image, output_width, output_height);
  sgiimg_set_rle (image);
  if (title) sgiimg_set_name (image, title);

  image->file = outfile;
  if (! sgiimg_file_init (image))
    yyerror ("could not create the SGI image output file");
}


/*------------------------------------------------------------*/
void
sgii_finish_output (void)
{
  int rownum;
  unsigned char *row;

  image_render();

  row = malloc (output_width * sizeof (unsigned char));

  for (rownum = 0; rownum < output_height; rownum++) {
    glReadPixels (0, rownum, output_width, 1, GL_RED, GL_UNSIGNED_BYTE, row);
    sgiimg_write_next_char_row (image, row);
  }

  for (rownum = 0; rownum < output_height; rownum++) {
    glReadPixels (0, rownum, output_width, 1, GL_GREEN, GL_UNSIGNED_BYTE, row);
    sgiimg_write_next_char_row (image, row);
  }

  for (rownum = 0; rownum < output_height; rownum++) {
    glReadPixels (0, rownum, output_width, 1, GL_BLUE, GL_UNSIGNED_BYTE, row);
    sgiimg_write_next_char_row (image, row);
  }

  free (row);

  sgiimg_file_close (image);

  image_close();
}
