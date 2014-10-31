/* eps_img.c

   MolScript v2.1.2

   Encapsulated PostScript (EPS) image file.

   This implementation relies on the 'image.c' code, and therefore
   implicitly on OpenGL and GLX.

   Copyright (C) 1997-1998 Per Kraulis
    13-Sep-1997  working
*/

#include <assert.h>
#include <stdlib.h>

#include <GL/gl.h>

#include "eps_img.h"
#include "global.h"
#include "graphics.h"
#include "image.h"
#include "opengl.h"


/*============================================================*/
static int components = 3;
static float scale = 1.0;


/*------------------------------------------------------------*/
void
eps_set (void)
{
  ogl_set();

  output_first_plot = eps_first_plot;
  output_finish_output = eps_finish_output;
  output_start_plot = ogl_start_plot_general;

  output_pickable = NULL;

  output_mode = EPS_MODE;
}


/*------------------------------------------------------------*/
void
eps_first_plot (void)
{
  image_first_plot();
  set_outfile ("w");

  if (fprintf (outfile, "%%!PS-Adobe-3.0 EPSF-3.0\n") < 0)
    yyerror ("could not write to the output EPS file");

  fprintf (outfile, "%%%%BoundingBox: 0 0 %i %i\n",
	            (int) (scale * output_width + 0.49999),
	            (int) (scale * output_height + 0.4999));
  if (title) fprintf (outfile, "%%%%Title: %s\n", title);
  fprintf (outfile, "%%%%Creator: %s, %s\n", program_str, copyright_str);
  if (user_str[0] != '\0') fprintf (outfile, "%%%%For: %s\n", user_str);
  PRINT ("%%EndComments\n");
  PRINT ("%%BeginProlog\n");
  PRINT ("10 dict begin\n");
  PRINT ("/bwproc {\n");
  PRINT ("  rgbproc\n");
  PRINT ("  dup length 3 idiv string 0 3 0\n");
  PRINT ("  5 -1 roll {\n");
  PRINT ("  add 2 1 roll 1 sub dup 0 eq\n");
  PRINT ("  { pop 3 idiv 3 -1 roll dup 4 -1 roll dup\n");
  PRINT ("    3 1 roll 5 -1 roll put 1 add 3 0 }\n");
  PRINT ("  { 2 1 roll } ifelse\n");
  PRINT ("  } forall\n");
  PRINT ("  pop pop pop\n");
  PRINT ("} def\n");
  PRINT ("systemdict /colorimage known not {\n");
  PRINT ("  /colorimage {\n");
  PRINT ("    pop pop\n");
  PRINT ("    /rgbproc exch def\n");
  PRINT ("    { bwproc } image\n");
  PRINT ("  } def\n");
  PRINT ("} if\n");
  fprintf (outfile, "/picstr %i string def\n", components * output_width);
  PRINT ("%%EndProlog\n");
  PRINT ("%%BeginSetup\n");
  PRINT ("gsave\n");
  fprintf (outfile, "%g %g scale\n",
	   scale * (float) output_width, scale * (float) output_height);
  PRINT ("%%EndSetup\n");
  fprintf (outfile, "%i %i 8\n", output_width, output_height);
  fprintf (outfile, "[%i 0 0 %i 0 0]\n", output_width, output_height);
  PRINT ("{currentfile picstr readhexstring pop}\n");
  fprintf (outfile, "false %i\n", components);
  fprintf (outfile, "%%%%BeginData: %i Hex Bytes\n",
	            2 * output_width * output_height * components + 11);
  PRINT ("colorimage\n");
}


/*------------------------------------------------------------*/
void
eps_finish_output (void)
{
  int byte_count = output_width * components;
  int row, pos, slot;
  GLenum format;
  unsigned char *buffer;
  char *pix;

  format = (components == 1) ? GL_LUMINANCE : GL_RGB;

  image_render();

  buffer = malloc (byte_count * sizeof (unsigned char));

  for (row = 0; row < output_height; row++) {
    glReadPixels (0, row, output_width, 1, format, GL_UNSIGNED_BYTE, buffer);
    pos = 0;
    pix = (char *) buffer;
    for (slot = 0; slot < byte_count; slot++) {
      fprintf (outfile, "%02hx", *pix++);
      if (++pos >= 32) {
	fprintf (outfile, "\n");
	pos = 0;
      }
    }
    if (pos) fprintf (outfile, "\n");
  }

  free (buffer);

  PRINT ("%%EndData\n");
  PRINT ("grestore\n");
  PRINT ("end\n");

  image_close();
}


/*------------------------------------------------------------*/
void
eps_set_bw (void)
{
  components = 1;
}


/*------------------------------------------------------------*/
void
eps_set_scale (float new)
{
  assert (new > 0.0);

  scale = new;
}
