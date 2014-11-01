/* image.c

   MolScript v2.1.2

   Image file general output routines.

   This implementation requires OpenGL and GLX (X windowing system).
   It relies on the 'opengl.c' code.

   Copyright (C) 1997-1998 Per Kraulis
    11-Sep-1997  split out of jpeg.c
    14-Sep-1997  finished
    12-Mar-1998  fixed number of required buffer bits
    19-Aug-1998  implemented GLX Pbuffer extension
    23-Nov-1998  got rid of GLX Pbuffer extension; fixed visual depth bug
*/

#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include "image.h"
#include "global.h"
#include "graphics.h"
#include "opengl.h"


/*============================================================*/
static Display *dpy;
static XVisualInfo *vis;
static GLXContext ctx;
static Bool out_of_memory;
static XErrorHandler old_error_handler;
static Pixmap xpixmap;
static GLXPixmap glxpixmap;


/*------------------------------------------------------------*/
static void
cleanup (int level, char *msg)
{
  switch (level) {
  case 5:
    glXMakeCurrent (dpy, None, NULL);
  case 4:
    glXDestroyContext (dpy, ctx);
  case 3:
    glXDestroyGLXPixmap (dpy, glxpixmap);
  case 2:
    XFreePixmap (dpy, xpixmap);
  case 1:
    XFree (vis);
  case 0:
    XCloseDisplay (dpy);
  }

  if (msg) yyerror (msg);
}


/*------------------------------------------------------------*/
static int
x_error_handler (Display *dpy, XErrorEvent *evt)
{
  out_of_memory = True;
  return 0;
}


/*------------------------------------------------------------*/
static void
visual_alternative (int alt, int *attributes)
{
  int count = 0;

  switch (alt) {

  case 0:
    attributes[count++] = GLX_RGBA;
    attributes[count++] = GLX_RED_SIZE;
    attributes[count++] = 8;
    attributes[count++] = GLX_GREEN_SIZE;
    attributes[count++] = 8;
    attributes[count++] = GLX_BLUE_SIZE;
    attributes[count++] = 8;
    attributes[count++] = GLX_DEPTH_SIZE;
    attributes[count++] = 1;
    if (ogl_accum() != 0) {
      attributes[count++] = GLX_ACCUM_RED_SIZE;
      attributes[count++] = 16;
      attributes[count++] = GLX_ACCUM_GREEN_SIZE;
      attributes[count++] = 16;
      attributes[count++] = GLX_ACCUM_BLUE_SIZE;
      attributes[count++] = 16;
    }
    break;

  case 1:
    attributes[count++] = GLX_RGBA;
    attributes[count++] = GLX_DOUBLEBUFFER;
    attributes[count++] = GLX_RED_SIZE;
    attributes[count++] = 8;
    attributes[count++] = GLX_GREEN_SIZE;
    attributes[count++] = 8;
    attributes[count++] = GLX_BLUE_SIZE;
    attributes[count++] = 8;
    attributes[count++] = GLX_DEPTH_SIZE;
    attributes[count++] = 1;
    if (ogl_accum() != 0) {
      attributes[count++] = GLX_ACCUM_RED_SIZE;
      attributes[count++] = 16;
      attributes[count++] = GLX_ACCUM_GREEN_SIZE;
      attributes[count++] = 16;
      attributes[count++] = GLX_ACCUM_BLUE_SIZE;
      attributes[count++] = 16;
    }
    break;

  case 2:
    attributes[count++] = GLX_RGBA;
    attributes[count++] = GLX_RED_SIZE;
    attributes[count++] = 1;
    attributes[count++] = GLX_GREEN_SIZE;
    attributes[count++] = 1;
    attributes[count++] = GLX_BLUE_SIZE;
    attributes[count++] = 1;
    attributes[count++] = GLX_DEPTH_SIZE;
    attributes[count++] = 1;
    if (ogl_accum() != 0) {
      attributes[count++] = GLX_ACCUM_RED_SIZE;
      attributes[count++] = 1;
      attributes[count++] = GLX_ACCUM_GREEN_SIZE;
      attributes[count++] = 1;
      attributes[count++] = GLX_ACCUM_BLUE_SIZE;
      attributes[count++] = 1;
    }
    break;

  case 3:
    attributes[count++] = GLX_RGBA;
    attributes[count++] = GLX_DOUBLEBUFFER;
    attributes[count++] = GLX_RED_SIZE;
    attributes[count++] = 1;
    attributes[count++] = GLX_GREEN_SIZE;
    attributes[count++] = 1;
    attributes[count++] = GLX_BLUE_SIZE;
    attributes[count++] = 1;
    attributes[count++] = GLX_DEPTH_SIZE;
    attributes[count++] = 1;
    if (ogl_accum() != 0) {
      attributes[count++] = GLX_ACCUM_RED_SIZE;
      attributes[count++] = 1;
      attributes[count++] = GLX_ACCUM_GREEN_SIZE;
      attributes[count++] = 1;
      attributes[count++] = GLX_ACCUM_BLUE_SIZE;
      attributes[count++] = 1;
    }
    break;

  default:
    yyerror ("internal: invalid visual alternative");
  }

  attributes[count++] = (int) None;
}


/*------------------------------------------------------------*/
void
image_first_plot (void)
{
  int attributes[32];
  int slot, value;
  GLboolean bparam;

  if (! first_plot)
    yyerror ("only one plot per input file allowed for image file output");

  dpy = XOpenDisplay (NULL);
  if (dpy == NULL) yyerror ("X display could not be opened");
  if (! glXQueryExtension (dpy, NULL, NULL))
    cleanup (0, "X server does not support OpenGL GLX extension");

  for (slot = 0; slot <= 3; slot++) {
    visual_alternative (slot, attributes);
    vis = glXChooseVisual (dpy, DefaultScreen (dpy), attributes);
    if (vis != NULL) break;
  }

  if (vis == NULL) {
    cleanup (0,
	     (ogl_accum() != 0) ?
	     "X server has no accumulation TrueColor GLX visual" :
	     "X server has no TrueColor GLX visual");
  }

  out_of_memory = False;
  old_error_handler = XSetErrorHandler (x_error_handler);
  xpixmap = XCreatePixmap (dpy, RootWindow (dpy, vis->screen),
			   output_width, output_height, vis->depth);
  XSync (dpy, False);
  if (out_of_memory) {
    XSetErrorHandler (old_error_handler);
    cleanup (1, "X server could not allocate the X Pixmap");
  }

  glxpixmap = glXCreateGLXPixmap (dpy, vis, xpixmap);
  XSync (dpy, False);
  XSetErrorHandler (old_error_handler);
  if (out_of_memory) cleanup (2, "X server could not allocate the GLX Pixmap");

  ctx = glXCreateContext (dpy, vis, NULL, False);
  if (ctx == NULL)
    cleanup (3, "X server could not create the GLX Context");

  if (! glXMakeCurrent (dpy, glxpixmap, ctx))
    cleanup (4, "X server could not make the GLX Pixmap and Context current");

  glGetBooleanv (GL_DOUBLEBUFFER, &bparam);
  if (bparam == GL_TRUE) {
    glDrawBuffer (GL_FRONT);
    glReadBuffer (GL_FRONT);
  }
}


/*------------------------------------------------------------*/
void
image_render (void)
{
  ogl_render_init();
  ogl_render_lights();
  ogl_render_lists();
  glDisable(GL_BLEND);		/* optimize pixel transfer rates */
  glDisable (GL_DEPTH_TEST);
  glDisable (GL_DITHER);
  glDisable (GL_FOG);
  glDisable (GL_LIGHTING);
  glFinish();
}


/*------------------------------------------------------------*/
void
image_close (void)
{
  cleanup (5, NULL);
}
