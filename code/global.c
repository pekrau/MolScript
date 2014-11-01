/* global.c

   MolScript v2.1.2

   Global stuff.

   Copyright (C) 1997-1998 Per Kraulis
     1-Dec-1996  first attempts
*/

#include <assert.h>
#include <string.h>

#include "clib/args.h"
#include "clib/str_utils.h"

#include "global.h"
#include "lex.h"
#include "state.h"
#include "graphics.h"
#include "xform.h"
#include "postscript.h"
#include "raster3d.h"
#include "vrml.h"

#ifdef OPENGL_SUPPORT
#include "opengl.h"
#ifdef IMAGE_SUPPORT
#include "image.h"
#include "eps_img.h"
#include "sgi_img.h"
#ifdef JPEG_SUPPORT
#include "jpeg_img.h"
#endif
#ifdef PNG_SUPPORT
#include "png_img.h"
#endif
#ifdef GIF_SUPPORT
#include "gif_img.h"
#endif
#endif
#endif


/*------------------------------------------------------------*/
const char program_str[] = "MolScript v2.1.2";
const char copyright_str[] = "Copyright (C) 1997-1998 Per J. Kraulis";
char user_str[81];

int output_mode = UNDEFINED_MODE;

char *input_filename = NULL;
char *output_filename = NULL;
char *tmp_filename = NULL;
FILE *outfile;
boolean message_mode = TRUE;
boolean exit_on_error = TRUE;
boolean pretty_format = FALSE;
int output_width = 500;
int output_height = 500;

double dstack [MAX_DSTACK];
int dstack_size;
int ival;

vector3 xaxis = {1.0, 0.0, 0.0};
vector3 yaxis = {0.0, 1.0, 0.0};
vector3 zaxis = {0.0, 0.0, 1.0};

char *title = NULL;
boolean first_plot;


/*------------------------------------------------------------*/
void
banner (void)
{
  char *prefix = "-----";

  if (message_mode) {
    fprintf (stderr, "%s %s, %s\n", prefix, program_str, copyright_str);
    fprintf (stderr,
	     "%s ref: P.J. Kraulis, J. Appl. Cryst. (1991) vol 24, pp 946-950\n",
	     prefix);
    fprintf (stderr, "%s http://www.avatar.se/molscript/\n", prefix);
  }
}


/*------------------------------------------------------------*/
void
global_init (void)
{
  outfile = stdout;
  if (title) {
    free (title);
    title = NULL;
  }
  first_plot = TRUE;
  xform_init_stored();
  if (getenv ("USER") != NULL) {
    strncpy (user_str, getenv ("USER"), 80);
  } else {
    user_str[0] = '\0';
  }
}


/*------------------------------------------------------------*/
void
push_double (double d)
{
  assert (dstack_size < MAX_DSTACK);

  dstack[dstack_size++] = d;
}


/*------------------------------------------------------------*/
void
clear_dstack (void)
{
  dstack_size = 0;
}


/*------------------------------------------------------------*/
void
pop_dstack (int slots)
{
  assert (slots > 0);

  dstack_size -= slots;

  assert (dstack_size >= 0);
}


/*------------------------------------------------------------*/
void
do_nothing (void)
{
  clear_dstack();
}


/*------------------------------------------------------------*/
void
do_nothing_str (char *str)
{
  clear_dstack();
}


/*------------------------------------------------------------*/
void
not_implemented (const char *str)
{
  assert (str);

  fprintf (stderr, "feature %s not implemented\n", str);
}


/*------------------------------------------------------------*/
static void
argument_error (const char *msg, int slot)
{
  assert (msg);

  if (slot >= 0) {
    fprintf (stderr, "Error: %s: %s\n", msg, args_item (slot));
  } else {
    fprintf (stderr, "Error: %s\n", msg);
  }
  exit (1);
}


/*------------------------------------------------------------*/
void
process_arguments (int *argcp, char *argv[])
{
  int slot, slot2;
  char *str;

  args_initialize (*argcp, argv);
  args_flag (0);

  slot = args_exists ("-h");
  if (slot) {
    fprintf (stderr, "Usage: molscript [options] < script > outfile\n");
    fprintf (stderr, "-ps -postscript      PostScript file output (default)\n");
    fprintf (stderr, "-r -raster3d [alias] Raster3D file output (for 'render'), alias=1,2,3\n");
    fprintf (stderr, "-vrml                VRML 2.0 file output\n");
#ifdef OPENGL_SUPPORT
    fprintf (stderr, "-gl -opengl          OpenGL interactive graphics output (no file)\n");
#ifdef IMAGE_SUPPORT
    fprintf (stderr, "-eps [scale]         Encapsulated PS image file output, scale>0.0 (default 1.0)\n");
    fprintf (stderr, "-epsbw [scale]       Encapsulated PS black-and-white image file output\n");
    fprintf (stderr, "-sgi -rgb            SGI (aka RGB) image file output\n");
#ifdef JPEG_SUPPORT
    fprintf (stderr, "-jpeg [quality]      JPEG image file output, quality=1-100 (default 90)\n");
#endif
#ifdef PNG_SUPPORT
    fprintf (stderr, "-png [compress]      PNG image file output, compress=default,none,speed,size\n");
#endif
#ifdef GIF_SUPPORT
    fprintf (stderr, "-gif                 GIF image file output\n");
#endif
#endif
    fprintf (stderr, "-accum number        image accumulation steps, number>=1 (only OpenGL & images)\n");
#endif
    fprintf (stderr, "-pretty              nicely formatted output (VRML only)\n");
    fprintf (stderr, "-size width height   size of output image (pixels; default 500 500)\n");
    fprintf (stderr, "-s -silent           silent execution; no messages\n");
    fprintf (stderr, "-out filename        output to the named file, instead of stdout\n");
    fprintf (stderr, "-in filename         input from the named file, instead of stdin\n");
    fprintf (stderr, "-log filename        messages to the named log file, instead of stderr\n");
    fprintf (stderr, "-tmp filename        temporary file to use, if needed\n");
    fprintf (stderr, "-h                   output this message\n");
    banner();
    exit (0);
  }

  slot = args_exists ("-ps");
  slot2 = args_exists ("-postscript");
  if (slot || slot2) {
    if (output_mode != UNDEFINED_MODE) goto format_error;
    if (slot) args_flag (slot);
    if (slot2) args_flag (slot2);
    ps_set();
  }

  slot = args_exists ("-r");
  slot2 = args_exists ("-raster3d");
  if (slot || slot2) {
    if (output_mode != UNDEFINED_MODE) goto format_error;
    r3d_set();
    if (slot) args_flag (slot);
    if (slot2) {
      args_flag (slot2);
      slot = slot2;
    }
    str = args_item (slot + 1);
    if (str) {
      int antialiasing;
      if (sscanf (str, "%i", &antialiasing) == 1) {
	if (antialiasing < 1 || antialiasing > 3)
	  argument_error ("invalid antialiasing value for option -raster3d", slot + 1);
	args_flag (slot + 1);
	r3d_set_antialiasing (antialiasing);
      }
    }
  }

  slot = args_exists ("-vrml");
  if (slot) {
    if (output_mode != UNDEFINED_MODE) goto format_error;
    args_flag (slot);
    vrml_set();
  }

#ifdef OPENGL_SUPPORT

  slot = args_exists ("-gl");
  slot2 = args_exists ("-opengl");
  if (slot || slot2) {
    if (output_mode != UNDEFINED_MODE) goto format_error;
    if (slot) args_flag (slot);
    if (slot2) args_flag (slot2);
    ogl_set();
  }

#ifdef IMAGE_SUPPORT

  slot = args_exists ("-eps");
  slot2 = args_exists ("-epsbw");
  if (slot || slot2) {
    if (output_mode != UNDEFINED_MODE) goto format_error;
    eps_set();
    if (slot) args_flag (slot);
    if (slot2) {
      args_flag (slot2);
      eps_set_bw();
      slot = slot2;
    }
    str = args_item (slot + 1);
    if (str) {
      float scale;
      if (sscanf (str, "%g", &scale) == 1) {
	if (scale <= 0.0) 
	  argument_error ("invalid scale value for option -eps", slot + 1);
	args_flag (slot + 1);
	eps_set_scale (scale);
      }
    }
  }

  slot = args_exists ("-sgi");
  slot2 = args_exists ("-rgb");
  if (slot || slot2) {
    if (output_mode != UNDEFINED_MODE) goto format_error;
    if (slot) args_flag (slot);
    if (slot2) args_flag (slot2);
    sgii_set();
  }

#ifdef JPEG_SUPPORT
  slot = args_exists ("-jpeg");
  if (slot) {
    if (output_mode != UNDEFINED_MODE) goto format_error;
    args_flag (slot);
    jpgi_set();
    str = args_item (slot + 1);
    if (str) {
      int quality;
      if (sscanf (str, "%i", &quality) == 1) {
	if (quality <= 0 || quality > 100)
	  argument_error ("invalid quality value for option -jpeg", slot + 1);
	args_flag (slot + 1);
	jpgi_set_quality (quality);
      }
    }
  }
#endif /* JPEG_SUPPORT */

#ifdef PNG_SUPPORT
  slot = args_exists ("-png");
  if (slot) {
    if (output_mode != UNDEFINED_MODE) goto format_error;
    args_flag (slot);
    pngi_set();
    str = args_item (slot + 1);
    if (str && pngi_set_compression (str)) args_flag (slot + 1);
  }
#endif /* PNG_SUPPORT */

#ifdef GIF_SUPPORT
  slot = args_exists ("-gif");
  if (slot) {
    if (output_mode != UNDEFINED_MODE) goto format_error;
    if (slot) args_flag (slot);
    gifi_set();
  }
#endif /*GIF_SUPPORT */

#endif /* IMAGE_SUPPORT */

  slot = args_exists ("-accum");
  if (slot) {
    int number;
    args_flag (slot);
    str = args_item (slot + 1);
    if (str) {
      if ((sscanf (str, "%i", &number) != 1) || (number <= 0)) {
	argument_error ("invalid number for option -accum", slot + 1);
      } else {
	ogl_set_accum (number);
	args_flag (slot + 1);
      }
    } else {
      argument_error ("no number given for option -accum", -1);
    }
  }

#endif /* OPENGL_SUPPORT */

  slot = args_exists ("-pretty");
  if (slot) {
    args_flag (slot);
    pretty_format = TRUE;
  }

  slot = args_exists ("-size");
  if (slot) {
    args_flag (slot);
    str = args_item (slot + 1);
    if (str) {
      if ((sscanf (str, "%i", &output_width) != 1) ||
	  output_width < 1 || output_width > 4096)
	argument_error ("invalid width for option -size", slot + 1);
      args_flag (slot + 1);
      str = args_item (slot + 2);
      if (str) {
	if ((sscanf (str, "%i", &output_height) != 1) ||
	    output_height < 1 || output_height > 4096)
	  argument_error ("invalid height for option -size", slot + 2);
	args_flag (slot + 2);
      } else {
	argument_error ("no height given for option -size", -1);
      }
    } else {
      argument_error ("no width given for option -size", -1);
    }
  }

  slot = args_exists ("-s");
  slot2 = args_exists ("-silent");
  if (slot || slot2) {
    if (slot) args_flag (slot);
    if (slot2) args_flag (slot2);
    message_mode = FALSE;
  }

  slot = args_exists ("-out");
  if (slot) {
    str = args_item (slot + 1);
    args_flag (slot);
    if (str) {
      args_flag (slot + 1);
      output_filename = str;
    } else {
      argument_error ("no filename given for option -out", -1);
    }
  }

  slot = args_exists ("-in");
  if (slot) {
    args_flag (slot);
    input_filename = str_clone (args_item (slot + 1));
    if (input_filename) {
      lex_set_input_file (input_filename);
      if (lex_input_file() == NULL)
	argument_error ("could not open the input file", slot + 1);
      args_flag (slot + 1);
    } else {
      argument_error ("no filename given for option -in", -1);
    }
  }

  slot = args_exists ("-log");
  if (slot) {
    FILE *file;
    args_flag (slot);
    str = args_item (slot + 1);
    if (str) {
      file = freopen (str, "w", stderr);
      if (file == NULL)
	argument_error ("could not open the log file", slot + 1);
      args_flag (slot + 1);
    } else {
      argument_error ("no filename given for option -log", -1);
    }
  }

  slot = args_exists ("-tmp");
  if (slot) {
    args_flag (slot);
    str = args_item (slot + 1);
    if (str) {
      tmp_filename = str;
      args_flag (slot + 1);
    } else {
      argument_error ("no filename given for option -tmp", -1);
    }
  }

  if (args_unflagged() >= 0)
    argument_error ("invalid command line option", args_unflagged());

  if (output_mode == UNDEFINED_MODE) ps_set();

  return;

format_error:
  argument_error ("more than one format options given", slot);
}


/*------------------------------------------------------------*/
void
set_outfile (const char *mode)
{
  assert (mode);
  assert (*mode);

  if (output_filename) {
    outfile = fopen (output_filename, mode);
    if (outfile == NULL) yyerror ("could not create the output file");
    output_filename = NULL;
  }
}


/*------------------------------------------------------------*/
void
set_title (const char *str)
{
  assert (str);
  assert (*str);

  title = str_clone (str);
}


/*------------------------------------------------------------*/
void
start_plot (void)
{
  output_first_plot();
  first_plot = FALSE;

  state_init();
  graphics_plot_init();
  delete_all_molecules();
  xform_init();
  clear_dstack();

  output_start_plot();
}


/*------------------------------------------------------------*/
void
debug (const char *str)
{
  assert (str);

  if (message_mode) fprintf (stderr, "%s\n", str);
}
