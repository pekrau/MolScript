/* png_img.c

   MolScript v2.1.2

   PNG image.

   This implementation is based on the PNG Reference Library 1.0
   version 0.96 and zlib v1.0.4. It relies on the 'image.c' code,
   and therefore implicitly on OpenGL and GLX.

   Copyright (C) 1997-1998 Per Kraulis
    12-Sep-1997  started
    21-Dec-1997  identified minor memory leak in PNG library; not fixed
*/

#include <assert.h>
#include <stdlib.h>

#include <GL/gl.h>

#include <png.h>

#include "clib/str_utils.h"
#include "clib/dynstring.h"

#include "png_img.h"
#include "global.h"
#include "graphics.h"
#include "image.h"
#include "opengl.h"


/*============================================================*/
static int compression_level = Z_DEFAULT_COMPRESSION;
static png_structp png_ptr;
static png_infop info_ptr;
static png_text text_ptr[4];


/*------------------------------------------------------------*/
void
pngi_set (void)
{
  ogl_set();

  output_first_plot = pngi_first_plot;
  output_finish_output = pngi_finish_output;
  output_start_plot = ogl_start_plot_general;

  output_pickable = NULL;

  output_mode = PNG_MODE;
}


/*------------------------------------------------------------*/
void
pngi_first_plot (void)
{
  int count = 0;
  dynstring *software_info;

  image_first_plot();
  set_outfile ("wb");

  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL)
    yyerror ("png_img: could not create PNG image structure");
  info_ptr = png_create_info_struct (png_ptr);
  if (info_ptr == NULL)
    yyerror ("png_img: could not create PNG info structure");
  if (setjmp (png_ptr->jmpbuf)) yyerror ("png_img: could not setjmp");

  png_init_io (png_ptr, outfile);
  png_set_compression_level (png_ptr, compression_level);
  png_set_IHDR (png_ptr, info_ptr, output_width, output_height, 8,
		PNG_COLOR_TYPE_RGB, NULL, NULL, NULL);

  if (title) {
    text_ptr[count].key = "Title";
    text_ptr[count].text = title;
    text_ptr[count++].compression = PNG_TEXT_COMPRESSION_NONE;
  }
  text_ptr[count].key = "Software";
  software_info = ds_create (program_str);
  ds_cat (software_info, ", ");
  ds_cat (software_info, copyright_str);
  text_ptr[count].text = software_info->string;
  text_ptr[count++].compression = PNG_TEXT_COMPRESSION_NONE;
  if (user_str[0] != '\0') {
    text_ptr[count].key = "Author";
    text_ptr[count].text = user_str;
    text_ptr[count++].compression = PNG_TEXT_COMPRESSION_NONE;
  }
  png_set_text (png_ptr, info_ptr, text_ptr, count);

  png_write_info (png_ptr, info_ptr);

  ds_delete (software_info);
}


/*------------------------------------------------------------*/
void
pngi_finish_output (void)
{
  int row;
  unsigned char *buffer;	/* this ought to be png_bytep? */

  image_render();

  buffer = malloc (output_width * 3 * sizeof (unsigned char));

  for (row = output_height - 1; row >= 0; row--) {
    glReadPixels (0, row, output_width, 1, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    png_write_row (png_ptr, (png_bytep) buffer);
  }

  free (buffer);

  png_write_end (png_ptr, NULL);
  png_destroy_write_struct (&png_ptr, &info_ptr);

  image_close();
}


/*------------------------------------------------------------*/
int
pngi_set_compression (char *level)
{
  assert (level);
  assert (*level);

  if (str_eq (level, "default")) {
    compression_level = Z_DEFAULT_COMPRESSION;
  } else if (str_eq (level, "speed")) {
    compression_level = Z_BEST_SPEED;
  } else if (str_eq (level, "size")) {
    compression_level = Z_BEST_COMPRESSION;
  } else if (str_eq (level, "none")) {
    compression_level = Z_NO_COMPRESSION;
  } else {
    return FALSE;
  }
  return TRUE;
}
