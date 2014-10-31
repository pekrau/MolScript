/* opengl.c

   MolScript v2.1.2

   OpenGL

   This implementation requires the GLUT library, v3.5 or later.

   Copyright (C) 1997-1998 Per Kraulis
     25-Jun-1997  started
     11-Sep-1997  fairly finished
     14-Jan-1998  fixed parse error crash bug in 'render_window'
     25-Jan-1998  face culling now default; object output param's optimized
      6-Feb-1998  use faster bitmap character routine
     21-Jul-1998  fixed strand arrow geometry bug
*/

#include <assert.h>
#include <stdlib.h>

#include <GL/glut.h>

#include "clib/str_utils.h"
#include "clib/dynstring.h"
#include "clib/quaternion.h"
#include "clib/extent3d.h"
#include "clib/ogl_utils.h"
#include "clib/ogl_body.h"
#include "clib/ogl_bitmap_character.h"

#include "other/jitter.h"

#include "opengl.h"
#include "lex.h"
#include "global.h"
#include "graphics.h"
#include "segment.h"
#include "state.h"
#include "select.h"


/*============================================================*/
static int ogl_window = 0;

static GLuint *display_list = NULL;
static int display_list_count = 0;
static int display_list_alloc = 0;
static GLuint *label_display_list = NULL;
static int label_display_list_count = 0;
static int label_display_list_alloc = 0;
static GLuint *alpha_display_list = NULL;
static int alpha_display_list_count = 0;
static int alpha_display_list_alloc = 0;
static GLuint latest_display_list = 0;

static boolean rereading = FALSE;

static int line_point_count;
static int polygon_count;
static int label_count;

enum menu_codes {MENU_FULL_SCREEN, MENU_NORMAL_WINDOW, MENU_OUTPUT_VIEW,
		 MENU_RESET_VIEW, MENU_READ_FILE, MENU_QUIT};

static quaternion rotation;
static double xform[4][4];
static double window_scale;
static double original_slab;

enum display_modes {VIEW_MODE, ROTATE_MODE, SCALE_MODE, PICK_MODE, SLAB_MODE};
static int display_mode, mouse_x, mouse_y;

#define MAX_LIGHTS 7
enum light_types {OGL_DIRECTIONALLIGHT, OGL_POINTLIGHT, OGL_SPOTLIGHT};
static int light_count;
static int light_type[MAX_LIGHTS];
static GLfloat light_position[MAX_LIGHTS][4];
static GLfloat light_spot_direction[MAX_LIGHTS][3];

static colour current_rgb;
static GLdouble current_alpha;
static double current_shininess;

static int accum = 0;
static jitter_point *jitter_points;

typedef struct {
  vector3 xyz;
  char *molname;
  char *resname;
  char *restype;
  char *atname;
} pickable;

static pickable *pickable_array = NULL;
static int pickable_count, pickable_alloc;

static GLuint *pick_buffer = NULL;
static GLsizei pick_buffer_alloc;


/*------------------------------------------------------------*/
static void
start_display_list (boolean is_label)
{
#ifndef NDEBUG
  GLint iparam[1];
  glGetIntegerv (GL_LIST_INDEX, iparam);
  assert (iparam[0] == 0);
#endif

  glNewList (++latest_display_list, GL_COMPILE);

  if (is_label) {
    if (label_display_list == NULL) {
      label_display_list_alloc = 128;
      label_display_list = malloc (label_display_list_alloc * sizeof (GLuint));
    } else if (label_display_list_count >= label_display_list_alloc) {
      label_display_list_alloc *= 2;
      label_display_list = realloc (label_display_list,
				    label_display_list_alloc * sizeof (GLuint));
    }
    label_display_list[label_display_list_count++] = latest_display_list;

  } else if (current_state->transparency != 0.0) {
    if (alpha_display_list == NULL) {
      alpha_display_list_alloc = 128;
      alpha_display_list = malloc (alpha_display_list_alloc * sizeof (GLuint));
    } else if (alpha_display_list_count >= alpha_display_list_alloc) {
      alpha_display_list_alloc *= 2;
      alpha_display_list = realloc (alpha_display_list,
				    alpha_display_list_alloc * sizeof (GLuint));
    }
    alpha_display_list[alpha_display_list_count++] = latest_display_list;

  } else {
    if (display_list == NULL) {
      display_list_alloc = 256;
      display_list = malloc (display_list_alloc * sizeof (GLuint));
    } else if (display_list_count >= display_list_alloc) {
      display_list_alloc *= 2;
      display_list = realloc (display_list,
				    display_list_alloc * sizeof (GLuint));
    }
    display_list[display_list_count++] = latest_display_list;
  }
}


/*------------------------------------------------------------*/
static void
set_colour_property (colour *c)
{
  assert (c);

  colour_copy_to_rgb (&current_rgb, c);
  current_alpha = 1.0 - current_state->transparency;
  glColor4d (current_rgb.x, current_rgb.y, current_rgb.z, current_alpha);
}


/*------------------------------------------------------------*/
static void
set_colour_property_if_different (colour *c)
{
  colour rgb;
  GLdouble alpha = 1.0 - current_state->transparency;

  assert (c);

  colour_copy_to_rgb (&rgb, c);
  if (colour_unequal (&rgb, &current_rgb) || (current_alpha != alpha)) {
    current_rgb = rgb;
    current_alpha = alpha;
    glColor4d (current_rgb.x, current_rgb.y, current_rgb.z, current_alpha);
  }
}


/*------------------------------------------------------------*/
static void
set_material_properties (void)
{
  colour rgb;
  GLfloat glcol[4];

  colour_copy_to_rgb (&rgb, &(current_state->specularcolour));
  if (colour_unequal (&rgb, &black_colour) &&
      current_shininess != current_state->shininess) {
    GLfloat sh = 128.0 * current_state->shininess;
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, sh);
    current_shininess = current_state->shininess;
  }
  glcol[0] = rgb.x;
  glcol[1] = rgb.y;
  glcol[2] = rgb.z;
  glcol[3] = 1.0;
  glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, glcol);

  colour_copy_to_rgb (&rgb, &(current_state->emissivecolour));
  glcol[0] = rgb.x;
  glcol[1] = rgb.y;
  glcol[2] = rgb.z;
  glcol[3] = 1.0;
  glMaterialfv (GL_FRONT_AND_BACK, GL_EMISSION, glcol);
}


/*------------------------------------------------------------*/
static void
set_light_properties (GLenum light)
{
  GLfloat light_colour[4];
  colour rgb;

  colour_copy_to_rgb (&rgb, &(current_state->lightcolour));

  light_colour[0] = (GLfloat) (current_state->lightintensity * rgb.x);
  light_colour[1] = (GLfloat) (current_state->lightintensity * rgb.y);
  light_colour[2] = (GLfloat) (current_state->lightintensity * rgb.z);
  light_colour[3] = 1.0;
  glLightfv (light, GL_DIFFUSE, light_colour);
  glLightfv (light, GL_SPECULAR, light_colour);

  light_colour[0] = (GLfloat) (current_state->lightambientintensity * rgb.x);
  light_colour[1] = (GLfloat) (current_state->lightambientintensity * rgb.y);
  light_colour[2] = (GLfloat) (current_state->lightambientintensity * rgb.z);
  light_colour[3] = 1.0;
  glLightfv (light, GL_AMBIENT, light_colour);

  glLightf (light, GL_CONSTANT_ATTENUATION,
	    (GLfloat) current_state->lightattenuation.x);
  glLightf (light, GL_LINEAR_ATTENUATION,
	    (GLfloat) current_state->lightattenuation.y);
  glLightf (light, GL_QUADRATIC_ATTENUATION,
	    (GLfloat) current_state->lightattenuation.z);
}


/*------------------------------------------------------------*/
static void
reshape_window (int width, int height)
{
  output_width = width;
  output_height = height;
  glViewport (0, 0, (GLsizei) output_width, (GLsizei) output_height);
}


/*------------------------------------------------------------*/
static void
pick (void)
{
  GLint viewport[4];
  int slot, hits;

  if (pickable_count == 0) return; /* otherwise an OpenGL error is generated */

  glSelectBuffer (pick_buffer_alloc, pick_buffer);
  glRenderMode (GL_SELECT);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity();

  viewport[0] = 0;
  viewport[1] = 0;
  viewport[2] = output_width;
  viewport[3] = output_height;
  gluPickMatrix ((GLdouble) mouse_x, (GLdouble) (viewport[3] - mouse_y),
		 8.0, 8.0, viewport);

  glOrtho (- aspect_window_x * window_scale, aspect_window_x * window_scale,
	   - aspect_window_y * window_scale, aspect_window_y * window_scale,
	   0.0, 2.0 * slab);

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt (0.0, 0.0, slab,
	     0.0, 0.0, 0.0,
	     0.0, 1.0, 0.0);

  quat_to_matrix3 (xform, &rotation);
  glMultMatrixd (&(xform[0][0]));

  glInitNames();
  glPushName (0);
  for (slot = 0; slot < pickable_count; slot++) {
    glLoadName (slot);
    glBegin (GL_POINTS);
    glVertex3d (pickable_array[slot].xyz.x,
		pickable_array[slot].xyz.y,
		pickable_array[slot].xyz.z);
    glEnd();
  }

  hits = glRenderMode (GL_RENDER);

  if (hits < 0) {		/* redo if overflow */
    pick_buffer_alloc *= 2;
    pick_buffer = realloc (pick_buffer, pick_buffer_alloc * sizeof (GLuint));
    pick();

  } else {
    int pos = 0;
    for (slot = 0; slot < hits; slot++) {
      assert (pick_buffer[pos] == 1);
      pos += 3;
      fprintf (stderr, "atom %s, residue %s %s, molecule %s\n",
	       pickable_array[pick_buffer[pos]].atname,
	       pickable_array[pick_buffer[pos]].resname,
	       pickable_array[pick_buffer[pos]].restype,
	       pickable_array[pick_buffer[pos]].molname);
      pos++;
    }
  }

#ifndef NDEBUG
  ogl_check_errors ("pick: ");
#endif
}


/*------------------------------------------------------------*/
static void
mouse_action (int button, int state, int x, int y)
{
  int modifiers = glutGetModifiers();

  if ((display_mode == VIEW_MODE) && (state == GLUT_DOWN)) {
    mouse_x = x;
    mouse_y = y;

    switch (button) {
    case GLUT_LEFT_BUTTON:
      if (modifiers & GLUT_ACTIVE_SHIFT) {
	display_mode = SCALE_MODE;
      } else {
	display_mode = ROTATE_MODE;
      }
      break;
    case GLUT_MIDDLE_BUTTON:
      if (modifiers & GLUT_ACTIVE_SHIFT) {
	display_mode = SLAB_MODE;
      } else {
	pick();
      }
      break;
    case GLUT_RIGHT_BUTTON:
      break;
    }

  } else if ((display_mode != VIEW_MODE) && (state == GLUT_UP)) {
    switch (button) {
    case GLUT_LEFT_BUTTON:
      if ((display_mode == ROTATE_MODE) || (display_mode == SCALE_MODE)) {
	display_mode = VIEW_MODE;
	if (accum != 0) glutPostRedisplay();
      }
      break;
    case GLUT_MIDDLE_BUTTON:
      if (display_mode == SLAB_MODE) {
	display_mode = VIEW_MODE;
	if (accum != 0) glutPostRedisplay();
      }
      break;
    case GLUT_RIGHT_BUTTON:
      break;
    }
  }
}


/*------------------------------------------------------------*/
static void
motion_action (int x, int y)
{
  quaternion increment;
  double p1x, p1y, p2x, p2y;

  switch (display_mode) {

  case ROTATE_MODE:
    if (x < 0) break;
    if (y < 0) break;
    if (x > output_width) break;
    if (y > output_height) break;
    p1x = (double) (2 * mouse_x - output_width) / (double) output_width;
    if (p1x > 1.0) p1x = 1.0; else if (p1x < -1.0) p1x = -1.0;
    p1y = (double) (output_height - 2 * mouse_y) / (double) output_height;
    if (p1y > 1.0) p1y = 1.0; else if (p1y < -1.0) p1y = -1.0;
    p2x = (double) (2 * x - output_width) / (double) output_width;
    if (p2x > 1.0) p2x = 1.0; else if (p2x < -1.0) p2x = -1.0;
    p2y = (double) (output_height - 2 * y) / (double) output_height;
    if (p2y > 1.0) p2y = 1.0; else if (p2y < -1.0) p2y = -1.0;
    quat_trackball (&increment, 0.8, p1x, p1y, p2x, p2y);
    quat_add (&rotation, &increment);
    quat_normalize (&rotation);
    quat_add (&rotation, &increment);
    quat_normalize (&rotation);
    mouse_x = x;
    mouse_y = y;
    glutPostRedisplay();
    break;

  case SCALE_MODE:
    if (x < 0) break;
    if (y < 0) break;
    if (x > output_width) break;
    if (y > output_height) break;
    window_scale *= 1.0 + 0.004 * (mouse_y - y);
    if (window_scale < 1.0e-4) window_scale = 1.0e-4;
    mouse_x = x;
    mouse_y = y;
    glutPostRedisplay();
    break;

  case SLAB_MODE:
    if (x < 0) break;
    if (y < 0) break;
    if (x > output_width) break;
    if (y > output_height) break;
    slab *= 1.0 + 0.004 * (mouse_y - y);
    if (slab < 1.0e-4) slab = 1.0e-4;
    mouse_x = x;
    mouse_y = y;
    glutPostRedisplay();
    break;

  default:
    break;
  }
}


/*------------------------------------------------------------*/
static void
keyboard_action (unsigned char key, int x, int y)
{
  if (key == '\033') {		/* Esc key */
    banner();
    exit (0);
  }
}


/*------------------------------------------------------------*/
static void
menu_action (int value)
{
  static int window_pos_x, window_pos_y, window_size_x, window_size_y;

  switch (value) {

  case MENU_FULL_SCREEN:
    window_pos_x = glutGet (GLUT_WINDOW_X);
    window_pos_y = glutGet (GLUT_WINDOW_Y);
    window_size_x = glutGet (GLUT_WINDOW_WIDTH);
    window_size_y = glutGet (GLUT_WINDOW_HEIGHT);
    glutPopWindow();
    glutFullScreen();
    glutChangeToMenuEntry (1, "Normal window", MENU_NORMAL_WINDOW);
    break;			/* glutPostRedisplay not needed */

  case MENU_NORMAL_WINDOW:
    glutPositionWindow (window_pos_x, window_pos_y);
    glutReshapeWindow (window_size_x, window_size_y);
    glutChangeToMenuEntry (1, "Full screen", MENU_FULL_SCREEN);
    break;			/* glutPostRedisplay not needed */

  case MENU_OUTPUT_VIEW:
    fprintf (stderr, "window %.2f;\n", 2.0 * window_scale * window);
    fprintf (stderr, "slab %.2f;\n", 2.0 * slab);
    quat_to_matrix3 (xform, &rotation);
    fprintf (stderr, "by rotation\n  %g %g %g\n  %g %g %g\n  %g %g %g\n",
	             xform[0][0], xform[1][0], xform[2][0],
	             xform[0][1], xform[1][1], xform[2][1],
	             xform[0][2], xform[1][2], xform[2][2]);
    break;

  case MENU_RESET_VIEW:
    quat_trackball (&rotation, 0.8, 0.0, 0.0, 0.0, 0.0);
    window_scale = 1.0;
    slab = original_slab;
    glutPostRedisplay();
    break;

  case MENU_READ_FILE:
    global_init();
    lex_init();
    lex_set_input_file (input_filename);
    rereading = TRUE;
    yyparse();
    rereading = FALSE;
    glutPostRedisplay();
    break;
      
  case MENU_QUIT:
    banner();
    exit (0);
    break;

  default:
    exit_on_error = TRUE;
    yyerror ("internal; invalid menu action code");
    break;
  }
}


/*------------------------------------------------------------*/
static void
render_window (void)
{
  if (window < 0.0) {		/* only if there has been a parse error */
    push_double (20.0);
    set_window();
  }
  if (slab < 0.0) {
    push_double (20.0);
    set_slab();
  }

  glPushAttrib (GL_ALL_ATTRIB_BITS);
  ogl_render_init();
  quat_to_matrix3 (xform, &rotation);
  glMultMatrixd (&(xform[0][0]));
  ogl_render_lights();
  ogl_render_lists();
  glPopAttrib();
  glutSwapBuffers();
}


/*------------------------------------------------------------*/
void
ogl_set (void)
{
  int proxy_argc = 0;		/* GLUT arguments disabled */
  static char *proxy_argv[] = {"molscript", NULL, NULL};

  if (getenv ("OPENGL_DIRECT") != NULL) {
    proxy_argv[1] = "-direct";
  } else if (getenv ("OPENGL_INDIRECT") != NULL) {
    proxy_argv[1] = "-indirect";
  }

  while (proxy_argv [proxy_argc] != NULL) proxy_argc++;
  glutInit (&proxy_argc, proxy_argv);

  output_first_plot = ogl_first_plot;
  output_finish_output = glutMainLoop;
  output_start_plot = ogl_start_plot;
  output_finish_plot = ogl_finish_plot;

  set_area = ogl_set_area;
  set_background = ogl_set_background;
  anchor_start = do_nothing_str;
  anchor_description = do_nothing_str;
  anchor_parameter = do_nothing_str;
  anchor_start_geometry = do_nothing;
  anchor_finish = do_nothing;
  lod_start = do_nothing;
  lod_finish = do_nothing;
  lod_start_group = do_nothing;
  lod_finish_group = do_nothing;
  viewpoint_start = do_nothing_str;
  viewpoint_output = do_nothing;
  output_directionallight = ogl_directionallight;
  output_pointlight = ogl_pointlight;
  output_spotlight = ogl_spotlight;
  output_comment = do_nothing_str;

  output_coil = ogl_coil;
  output_cylinder = ogl_cylinder;
  output_helix = ogl_helix;
  output_label = ogl_label;
  output_line = ogl_line;
  output_sphere = ogl_sphere;
  output_stick = ogl_stick;
  output_strand = ogl_strand;

  output_start_object = ogl_start_object;
  output_object = ogl_object;
  output_finish_object = ogl_finish_object;

  output_pickable = ogl_pickable;

  constant_colours_to_rgb();

  output_mode = OPENGL_MODE;
}


/*------------------------------------------------------------*/
void
ogl_first_plot (void)
{
  if (!first_plot)
    yyerror ("only one plot per input file allowed for OpenGL");

  if (!rereading) {
    glutInitWindowPosition (0, 0);
    glutInitWindowSize (output_width, output_height);
    if (accum != 0) {
      glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ACCUM);
    } else {
      glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    }
    if (!glutGet (GLUT_DISPLAY_MODE_POSSIBLE)) {
      if (accum != 0) {
	glutInitDisplayMode (GLUT_RGBA | GLUT_SINGLE | GLUT_DEPTH | GLUT_ACCUM);
      } else {
	glutInitDisplayMode (GLUT_RGBA | GLUT_SINGLE | GLUT_DEPTH);
      }
    }
    if (!glutGet (GLUT_DISPLAY_MODE_POSSIBLE)) {
      exit_on_error = TRUE;
      yyerror ("could not create an OpenGL window with the required buffers");
    }
  }
				/* if reread after an error */
  while (ext3d_depth() > 1) ext3d_pop (FALSE);
  while (count_atom_selections()) pop_atom_selection();
  while (count_residue_selections()) pop_residue_selection();

  exit_on_error = (input_filename == NULL);
}


/*------------------------------------------------------------*/
void
ogl_start_plot (void)
{
  dynstring *window_title;

  quat_trackball (&rotation, 0.8, 0.0, 0.0, 0.0, 0.0);
  display_mode = VIEW_MODE;

  glDeleteLists (1, (GLsizei) latest_display_list);
  display_list_count = 0;
  label_display_list_count = 0;
  alpha_display_list_count = 0;

  if (pickable_array) {
    int slot;
    for (slot = 0; slot < pickable_count; slot++) {
      free (pickable_array[slot].molname);
      free (pickable_array[slot].resname);
      free (pickable_array[slot].restype);
      free (pickable_array[slot].atname);
    }
  }
  pickable_count = 0;

  if (title) {
    window_title = ds_create (title);
  } else {
    window_title = ds_create (program_str);
    ds_cat (window_title, " OpenGL");
  }

  if (rereading) {
    glutSetWindowTitle (window_title->string);

  } else {
    ogl_window = glutCreateWindow (window_title->string);

    glutReshapeFunc (reshape_window);
    glutDisplayFunc (render_window);
    glutMouseFunc (mouse_action);
    glutMotionFunc (motion_action);
    glutKeyboardFunc (keyboard_action);

    glutCreateMenu (menu_action);
    glutAddMenuEntry ("Full screen", MENU_FULL_SCREEN); /* # 1 */
    glutAddMenuEntry ("Output view", MENU_OUTPUT_VIEW);
    glutAddMenuEntry ("Reset view", MENU_RESET_VIEW);
    if (input_filename) glutAddMenuEntry ("Re-read input file", MENU_READ_FILE);
    glutAddMenuEntry ("Quit", MENU_QUIT);
    glutAttachMenu (GLUT_RIGHT_BUTTON);
 
  }

  ds_delete (window_title);

  output_width = glutGet (GLUT_WINDOW_WIDTH);
  output_height = glutGet (GLUT_WINDOW_HEIGHT);

  ogl_start_plot_general();
}


/*------------------------------------------------------------*/
void
ogl_finish_plot (void)
{
  set_extent();
  original_slab = slab;

  glClearColor ((GLclampf) background_colour.x,
		(GLclampf) background_colour.y,
		(GLclampf) background_colour.z,
		0.0);

  if (headlight) {
    glEnable (GL_LIGHT0);
  } else {
    glDisable (GL_LIGHT0);
  }

  if (fog != 0.0) {
    GLfloat fogcolour[4];
    glEnable (GL_FOG);
    glFogi (GL_FOG_MODE, GL_LINEAR);
    fogcolour[0] = background_colour.x;
    fogcolour[1] = background_colour.y;
    fogcolour[2] = background_colour.z;
    fogcolour[3] = 1.0;
    glFogfv (GL_FOG_COLOR, fogcolour);
  } else {
    glDisable (GL_FOG);
  }

  if (message_mode) {
    fprintf (stderr, "%i lines/points, %i polygons, %i labels in %i display lists rendered.\n",
	     line_point_count, polygon_count, label_count,
	     display_list_count + alpha_display_list_count + label_display_list_count);

    if (! rereading) {
      GLint iparam;
      fprintf (stderr, "Image buffer bits:");
      glGetIntegerv (GL_RED_BITS, &iparam);
      fprintf (stderr, " red %i,", iparam);
      glGetIntegerv (GL_GREEN_BITS, &iparam);
      fprintf (stderr, " green %i,", iparam);
      glGetIntegerv (GL_BLUE_BITS, &iparam);
      fprintf (stderr, " blue %i. ", iparam);
      glGetIntegerv (GL_DEPTH_BITS, &iparam);
      fprintf (stderr, "Depth buffer bits: %i.\n", iparam);
      if (ogl_accum() != 0) {
	fprintf (stderr, "Accumulation buffer depth:");
	glGetIntegerv (GL_ACCUM_RED_BITS, &iparam);
	fprintf (stderr, " red %i,", iparam);
	glGetIntegerv (GL_ACCUM_GREEN_BITS, &iparam);
	fprintf (stderr, " green %i,", iparam);
	glGetIntegerv (GL_ACCUM_BLUE_BITS, &iparam);
	fprintf (stderr, " blue %i.\n", iparam);
      }
    }
  }
}


/*------------------------------------------------------------*/
void
ogl_start_plot_general (void)
{
  window_scale = 1.0;

  line_point_count = 0;
  polygon_count = 0;
  label_count = 0;
  light_count = 0;

  colour_copy_to_rgb (&background_colour, &black_colour);

  current_shininess = current_state->shininess;

  set_area_values (0.0, 0.0,
		   (double) (output_width - 1),
		   (double) (output_height - 1));

  if (! rereading) {
    GLfloat black_light[] = {0.0, 0.0, 0.0, 1.0};
    GLint iparam[1];
    GLfloat sh = 128.0 * current_shininess;

    glEnable (GL_DEPTH_TEST);
    glGetIntegerv (GL_RED_BITS, iparam);
    if (iparam[0] >= 8) glDisable (GL_DITHER);
    glEnable (GL_LIGHTING);
    glEnable (GL_COLOR_MATERIAL);
    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, black_light);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, sh);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
  }
}


/*------------------------------------------------------------*/
void
ogl_render_init (void)
{
  set_area_values (0.0, 0.0,
		   (double) (output_width - 1),
		   (double) (output_height - 1));

  assert (aspect_window_x > 0.0);
  assert (aspect_window_y > 0.0);
  assert (window_scale > 0.0);
  assert (slab > 0.0);

  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity();
  glOrtho (- aspect_window_x * window_scale, aspect_window_x * window_scale,
	   - aspect_window_y * window_scale, aspect_window_y * window_scale,
	   0.0, 2.0 * slab);

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt (0.0, 0.0, slab,
	     0.0, 0.0, 0.0,
	     0.0, 1.0, 0.0);

  if (fog != 0.0) {
    glFogi (GL_FOG_START, 0.0);
    glFogi (GL_FOG_END, fog);
  }

  glEnable (GL_CULL_FACE);

#ifndef NDEBUG
  ogl_check_errors ("ogl_render_init: ");
#endif
}


/*------------------------------------------------------------*/
void
ogl_render_lights (void)
{
  int slot;
  GLenum light_number;

  for (slot = 0; slot < light_count; slot++) {
    light_number = GL_LIGHT0 + (GLenum) (slot + 1);
    glEnable (light_number);
    glLightfv (light_number, GL_POSITION, light_position[slot]);
    if (light_type[slot] == OGL_SPOTLIGHT) {
      glLightfv (light_number, GL_SPOT_DIRECTION, light_spot_direction[slot]);
    }
  }
#ifndef NDEBUG
  ogl_check_errors ("ogl_render_lights: ");
#endif
}


/*------------------------------------------------------------*/
void
ogl_render_lists (void)
{
  int slot;

  if ((accum != 0) && (display_mode == VIEW_MODE)) {

    int point;
    GLfloat fraction = 1.0 / (GLfloat) accum;
    float xpix = 2.0 * aspect_window_x * window_scale / (float) output_width;
    float ypix = 2.0 * aspect_window_y * window_scale / (float) output_height;

    for (point = 0; point < accum; point++) {
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glPushMatrix();
      glTranslatef (xpix * jitter_points[point].x,
		    ypix * jitter_points[point].y, 0.0);

      for (slot = 0; slot < display_list_count; slot++) {
	glCallList (display_list[slot]);
      }

      if (label_display_list_count > 0) {
	glPopMatrix();		/* labels must not be jittered */
	glDisable (GL_LIGHTING);
	for (slot = 0; slot < label_display_list_count; slot++) {
	  glCallList (label_display_list[slot]);
	}
	glEnable (GL_LIGHTING);
	glPushMatrix();
	glTranslatef (xpix * jitter_points[point].x,
		      ypix * jitter_points[point].y, 0.0);
      }

      if (alpha_display_list_count > 0) {
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (slot = 0; slot < alpha_display_list_count; slot++) {
	  glCallList (alpha_display_list[slot]);
	}
	glDisable (GL_BLEND);
      }

      glPopMatrix();

      if (point == 0) {
	glAccum (GL_LOAD, fraction);
      } else {
	glAccum (GL_ACCUM, fraction);
      }
    }

    glAccum (GL_RETURN, 1.0);

  } else {

    for (slot = 0; slot < display_list_count; slot++) {
      glCallList (display_list[slot]);
    }

    if (label_display_list_count > 0) {
      glDisable (GL_LIGHTING);
      for (slot = 0; slot < label_display_list_count; slot++) {
	glCallList (label_display_list[slot]);
      }
      glEnable (GL_LIGHTING);
    }

    if (alpha_display_list_count > 0) {
      glEnable (GL_BLEND);
      glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      for (slot = 0; slot < alpha_display_list_count; slot++) {
	glCallList (alpha_display_list[slot]);
      }
      glDisable (GL_BLEND);
    }
  }

#ifndef NDEBUG
  ogl_check_errors ("ogl_render_lists: ");
#endif
}


/*------------------------------------------------------------*/
int
ogl_accum (void)
{
  return accum;
}


/*------------------------------------------------------------*/
void
ogl_set_accum (int number)
{
  assert (number > 0);

  if (number <= 2) {
    accum = 2;
    jitter_points = j2;
  } else if (number <= 3) {
    accum = 3;
    jitter_points = j3;
  } else if (number <= 4) {
    accum = 4;
    jitter_points = j4;
  } else if (number <= 8) {
    accum = 8;
    jitter_points = j8;
  } else if (number <= 15) {
    accum = 15;
    jitter_points = j15;
  } else if (number <= 24) {
    accum = 24;
    jitter_points = j24;
  } else {
    accum = 66;
    jitter_points = j66;
  }

  assert (accum > 0);
  assert (jitter_points);
}


/*------------------------------------------------------------*/
void
ogl_set_area (void)
{
  if (message_mode) fprintf (stderr, "ignoring 'area' for OpenGL output\n");
  clear_dstack();
}


/*------------------------------------------------------------*/
void
ogl_set_background (void)
{
  colour_copy_to_rgb (&background_colour, &given_colour);
}


/*------------------------------------------------------------*/
void
ogl_directionallight (void)
{
  assert ((dstack_size == 3) || (dstack_size == 6));

  if (light_count + 1 >= MAX_LIGHTS) {
    if (message_mode)
      fprintf (stderr,
	       "ignoring 'directionallight'; maximum number of lights reached\n");
    clear_dstack();
    return;
  }

  light_type[light_count] = OGL_DIRECTIONALLIGHT;

  if (dstack_size == 3) {
    light_position[light_count][0] = (GLfloat) - dstack[0];
    light_position[light_count][1] = (GLfloat) - dstack[1];
    light_position[light_count][2] = (GLfloat) - dstack[2];
  } else {
    light_position[light_count][0] = (GLfloat) (dstack[0] - dstack[3]);
    light_position[light_count][1] = (GLfloat) (dstack[1] - dstack[4]);
    light_position[light_count][2] = (GLfloat) (dstack[2] - dstack[5]);
  }
  light_position[light_count][3] = 0.0;
  clear_dstack();

  set_light_properties ((GLenum) GL_LIGHT1 + light_count);

  light_count++;

  assert (dstack_size == 0);
}


/*------------------------------------------------------------*/
void
ogl_pointlight (void)
{
  assert (dstack_size == 3);

  if (light_count + 1 >= MAX_LIGHTS) {
    if (message_mode)
      fprintf (stderr,
	       "ignoring 'pointlight'; maximum number of lights reached\n");
    clear_dstack();
    return;
  }

  light_type[light_count] = OGL_POINTLIGHT;

  light_position[light_count][0] = (GLfloat) dstack[0];
  light_position[light_count][1] = (GLfloat) dstack[1];
  light_position[light_count][2] = (GLfloat) dstack[2];
  light_position[light_count][3] = 1.0;
  clear_dstack();

  set_light_properties ((GLenum) GL_LIGHT1 + light_count);

  light_count++;

  assert (dstack_size == 0);
}


/*------------------------------------------------------------*/
void
ogl_spotlight (void)
{
  assert ((dstack_size == 7) || (dstack_size == 10));

  if (light_count + 1 >= MAX_LIGHTS) {
    if (message_mode)
      fprintf (stderr,
	       "ignoring 'spotlight'; maximum number of lights reached\n");
    clear_dstack();
    return;
  }

  light_type[light_count] = OGL_SPOTLIGHT;

  light_position[light_count][0] = (GLfloat) dstack[0];
  light_position[light_count][1] = (GLfloat) dstack[1];
  light_position[light_count][2] = (GLfloat) dstack[2];
  light_position[light_count][3] = 1.0;

  if (dstack_size == 7) {
    light_spot_direction[light_count][0] = (GLfloat) dstack[3];
    light_spot_direction[light_count][1] = (GLfloat) dstack[4];
    light_spot_direction[light_count][2] = (GLfloat) dstack[5];
    glLightf (GL_LIGHT1 + (GLenum) light_count,
	      GL_SPOT_CUTOFF, (GLfloat) (dstack[6] / 2.0));
  } else {
    light_spot_direction[light_count][0] = (GLfloat) (dstack[6] - dstack[3]);
    light_spot_direction[light_count][1] = (GLfloat) (dstack[7] - dstack[4]);
    light_spot_direction[light_count][2] = (GLfloat) (dstack[8] - dstack[5]);
    glLightf (GL_LIGHT1 + (GLenum) light_count,
	      GL_SPOT_CUTOFF, (GLfloat) (dstack[9] / 2.0));
  }
  clear_dstack();

  set_light_properties (GL_LIGHT1 + (GLenum) light_count);

  light_count++;

  assert (dstack_size == 0);
}


/*------------------------------------------------------------*/
void
ogl_coil (void)
{
  int slot;
  vector3 normal;
  coil_segment *cs;

  start_display_list (FALSE);
  set_material_properties();

  if (current_state->colourparts) {
    colour rgb;

    glBegin (GL_QUADS);
    colour_to_rgb (&(coil_segments[0].c));
    rgb = coil_segments[0].c;
    set_colour_property (&rgb);
    v3_difference (&normal, &(coil_segments[0].p), &(coil_segments[1].p));
    v3_normalize (&normal);
    glNormal3d (normal.x, normal.y, normal.z);
    cs = coil_segments;
    glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
    glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
    glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
    glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
    glEnd();
    polygon_count++;

    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      colour_to_rgb (&(cs->c));
      if (colour_unequal (&rgb, &(cs->c))) {
	glNormal3d (cs->n1.x, cs->n1.y, cs->n1.z);
	glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
	glNormal3d (cs->n2.x, cs->n2.y, cs->n2.z);
	glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
	glEnd();
	glBegin (GL_TRIANGLE_STRIP);
	rgb = cs->c;
	set_colour_property (&rgb);
      }
      glNormal3d (cs->n1.x, cs->n1.y, cs->n1.z);
      glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
      glNormal3d (cs->n2.x, cs->n2.y, cs->n2.z);
      glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
    }
    glEnd();
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      if (colour_unequal (&rgb, &(cs->c))) {
	glNormal3d (cs->n2.x, cs->n2.y, cs->n2.z);
	glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
	glNormal3d (cs->n3.x, cs->n3.y, cs->n3.z);
	glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
	glEnd();
	glBegin (GL_TRIANGLE_STRIP);
	rgb = cs->c;
	set_colour_property (&rgb);
      }
      glNormal3d (cs->n2.x, cs->n2.y, cs->n2.z);
      glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
      glNormal3d (cs->n3.x, cs->n3.y, cs->n3.z);
      glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
    }
    glEnd();
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      if (colour_unequal (&rgb, &(cs->c))) {
	glNormal3d (cs->n3.x, cs->n3.y, cs->n3.z);
	glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
	glNormal3d (cs->n4.x, cs->n4.y, cs->n4.z);
	glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
	glEnd();
	glBegin (GL_TRIANGLE_STRIP);
	rgb = cs->c;
	set_colour_property (&rgb);
      }
      glNormal3d (cs->n3.x, cs->n3.y, cs->n3.z);
      glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
      glNormal3d (cs->n4.x, cs->n4.y, cs->n4.z);
      glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
    }
    glEnd();
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      if (colour_unequal (&rgb, &(cs->c))) {
	glNormal3d (cs->n4.x, cs->n4.y, cs->n4.z);
	glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
	glNormal3d (cs->n1.x, cs->n1.y, cs->n1.z);
	glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
	glEnd();
	glBegin (GL_TRIANGLE_STRIP);
	rgb = cs->c;
	set_colour_property (&rgb);
      }
      glNormal3d (cs->n4.x, cs->n4.y, cs->n4.z);
      glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
      glNormal3d (cs->n1.x, cs->n1.y, cs->n1.z);
      glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
    }
    glEnd();
    polygon_count += 8 * coil_segment_count;

    glBegin (GL_QUADS);
    v3_difference (&normal, &(coil_segments[coil_segment_count-1].p),
		            &(coil_segments[coil_segment_count-2].p));
    v3_normalize (&normal);
    glNormal3d (normal.x, normal.y, normal.z);
    cs = coil_segments + coil_segment_count - 1;
    glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
    glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
    glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
    glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
    glEnd();
    polygon_count++;

  } else {
    set_colour_property (&(current_state->planecolour));

    glBegin (GL_QUADS);
    v3_difference (&normal, &(coil_segments[0].p), &(coil_segments[1].p));
    v3_normalize (&normal);
    glNormal3d (normal.x, normal.y, normal.z);
    cs = coil_segments;
    glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
    glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
    glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
    glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
    glEnd();
    polygon_count++;

    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      glNormal3d (cs->n1.x, cs->n1.y, cs->n1.z);
      glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
      glNormal3d (cs->n2.x, cs->n2.y, cs->n2.z);
      glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
    }
    glEnd();
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      glNormal3d (cs->n2.x, cs->n2.y, cs->n2.z);
      glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
      glNormal3d (cs->n3.x, cs->n3.y, cs->n3.z);
      glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
    }
    glEnd();
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      glNormal3d (cs->n3.x, cs->n3.y, cs->n3.z);
      glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
      glNormal3d (cs->n4.x, cs->n4.y, cs->n4.z);
      glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
    }
    glEnd();
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      glNormal3d (cs->n4.x, cs->n4.y, cs->n4.z);
      glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
      glNormal3d (cs->n1.x, cs->n1.y, cs->n1.z);
      glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
    }
    glEnd();
    polygon_count += 8 * coil_segment_count;

    glBegin (GL_QUADS);
    v3_difference (&normal, &(coil_segments[coil_segment_count-1].p),
		            &(coil_segments[coil_segment_count-2].p));
    v3_normalize (&normal);
    glNormal3d (normal.x, normal.y, normal.z);
    cs = coil_segments + coil_segment_count - 1;
    glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
    glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
    glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
    glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
    glEnd();
    polygon_count++;
  }

  glEndList();
}


/*------------------------------------------------------------*/
void
ogl_cylinder (vector3 *v1, vector3 *v2)
{
  int segments = 3 * current_state->segments + 5;

  assert (v1);
  assert (v2);
  assert (v3_distance (v1, v2) > 0.0);

  start_display_list (FALSE);
  set_colour_property (&(current_state->planecolour));
  set_material_properties();
  ogl_cylinder_faces (v1, v2, current_state->cylinderradius, segments, TRUE);
  glEndList();
  polygon_count += 3 * segments;
}


/*------------------------------------------------------------*/
void
ogl_helix (void)
{
  int slot;
  colour rgb;
  helix_segment *hs;

  start_display_list (FALSE);
  glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  glDisable (GL_CULL_FACE);
  set_material_properties();

  if (current_state->colourparts) {

    glBegin (GL_TRIANGLE_STRIP);
    colour_to_rgb (&(helix_segments[0].c));
    rgb = helix_segments[0].c;
    set_colour_property (&rgb);
    for (slot = 0; slot < helix_segment_count; slot++) {
      hs = helix_segments + slot;
      colour_to_rgb (&(hs->c));
      if (colour_unequal (&rgb, &(hs->c))) {
	glNormal3d (hs->n.x, hs->n.y, hs->n.z);
	glVertex3d (hs->p1.x, hs->p1.y, hs->p1.z);
	glVertex3d (hs->p2.x, hs->p2.y, hs->p2.z);
	glEnd();
	glBegin (GL_TRIANGLE_STRIP);
	rgb = hs->c;
	set_colour_property (&rgb);
      }
      glNormal3d (hs->n.x, hs->n.y, hs->n.z);
      glVertex3d (hs->p1.x, hs->p1.y, hs->p1.z);
      glVertex3d (hs->p2.x, hs->p2.y, hs->p2.z);
    }
    glEnd();

  } else {			/* not colourparts */
    GLfloat glcol[4];

    glDisable (GL_COLOR_MATERIAL);

    colour_copy_to_rgb (&rgb, &(current_state->planecolour));
    glcol[0] = rgb.x;
    glcol[1] = rgb.y;
    glcol[2] = rgb.z;
    glcol[3] = 1.0 - current_state->transparency;
    glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, glcol);

    colour_copy_to_rgb (&rgb, &(current_state->plane2colour));
    glcol[0] = rgb.x;
    glcol[1] = rgb.y;
    glcol[2] = rgb.z;
    glcol[3] = 1.0 - current_state->transparency;
    glMaterialfv (GL_BACK, GL_AMBIENT_AND_DIFFUSE, glcol);

    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < helix_segment_count; slot++) {
      hs = helix_segments + slot;
      glNormal3d (hs->n.x, hs->n.y, hs->n.z);
      glVertex3d (hs->p1.x, hs->p1.y, hs->p1.z);
      glVertex3d (hs->p2.x, hs->p2.y, hs->p2.z);
    }
    glEnd();

    glEnable (GL_COLOR_MATERIAL);
  }

  glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
  glEnable (GL_CULL_FACE);
  glEndList();
  polygon_count += 2 * helix_segment_count;
}


/*------------------------------------------------------------*/
void
ogl_label (vector3 *p, char *label, colour *c)
{
  vector3 pos;

  assert (p);
  assert (label);
  assert (*label);

  start_display_list (TRUE);
  if (c) {
    set_colour_property (c);
  } else {
    set_colour_property (&(current_state->linecolour));
  }
  v3_sum (&pos, p, &(current_state->labeloffset));
  glRasterPos3d (pos.x, pos.y, pos.z);
  if ((output_width < 300) || (output_height < 300)) {
    if (current_state->labelsize <= 12.0) {
      ogl_bitmap_string (GLUT_BITMAP_HELVETICA_10, label);
    } else if (current_state->labelsize <= 20.0) {
      ogl_bitmap_string (GLUT_BITMAP_HELVETICA_12, label);
    } else {
      ogl_bitmap_string (GLUT_BITMAP_HELVETICA_18, label);
    }
  } else {
    if (current_state->labelsize <= 11.0) {
      ogl_bitmap_string (GLUT_BITMAP_HELVETICA_10, label);
    } else if (current_state->labelsize <= 15.0) {
      ogl_bitmap_string (GLUT_BITMAP_HELVETICA_12, label);
    } else {
      ogl_bitmap_string (GLUT_BITMAP_HELVETICA_18, label);
    }
  }
  glEndList();
  label_count++;
}


/*------------------------------------------------------------*/
void
ogl_line (boolean polylines)
{
  int slot;
  line_segment *ls;

  if (line_segment_count < 2) return;

  start_display_list (FALSE);
  glDisable (GL_LIGHTING);
  glShadeModel (GL_FLAT);
  glLineWidth (current_state->linewidth);
  if (current_state->linedash > 0.5) {
    glLineStipple ((GLint) (current_state->linedash + 0.5), 0xAAAA);
    glEnable (GL_LINE_STIPPLE);
  }

  if (current_state->colourparts) {

    if (polylines) {

      glBegin (GL_LINE_STRIP);
      set_colour_property (&(line_segments[0].c));
      glVertex3d (line_segments->p.x, line_segments->p.y, line_segments->p.z);
      for (slot = 1; slot < line_segment_count; slot++) {
	ls = line_segments + slot;
	if (ls->new) {
	  glEnd();
	  glBegin (GL_LINE_STRIP);
	}
	set_colour_property_if_different (&(ls->c));
	glVertex3d (ls->p.x, ls->p.y, ls->p.z);
      }
      glEnd();

    } else {			/* not polylines */

      glBegin (GL_LINES);
      set_colour_property (&(line_segments[0].c));
      for (slot = 0; slot < line_segment_count; slot += 2) {
	ls = line_segments + slot;
	set_colour_property_if_different (&(ls->c));
	glVertex3d (ls->p.x, ls->p.y, ls->p.z);
	ls++;
	glVertex3d (ls->p.x, ls->p.y, ls->p.z);
      }
      glEnd();
    }

  } else {			/* not colourparts */

    set_colour_property (&(line_segments[0].c));

    if (polylines) {

      glBegin (GL_LINE_STRIP);
      glVertex3d (line_segments->p.x, line_segments->p.y, line_segments->p.z);
      for (slot = 1; slot < line_segment_count; slot++) {
	ls = line_segments + slot;
	if (ls->new) {
	  glEnd();
	  glBegin (GL_LINE_STRIP);
	}
	glVertex3d (ls->p.x, ls->p.y, ls->p.z);
      }
      glEnd();

    } else {			/* not polylines */

      glBegin (GL_LINES);
      for (slot = 0; slot < line_segment_count; slot++) {
	ls = line_segments + slot;
	glVertex3d (ls->p.x, ls->p.y, ls->p.z);
      }
      glEnd();
    }
  }

  if (current_state->linedash > 0.5) glDisable (GL_LINE_STIPPLE);
  glShadeModel (GL_SMOOTH);
  glEnable (GL_LIGHTING);
  glEndList();
  line_point_count += line_segment_count;
}


/*------------------------------------------------------------*/
void
ogl_sphere (at3d *at, double radius)
{
  assert (at);
  assert (radius > 0.0);

  start_display_list (FALSE);
  set_colour_property (&(at->colour));
  set_material_properties();
  ogl_sphere_faces_globe (&(at->xyz), radius, 2 * current_state->segments);
  glEndList();
  polygon_count += 2 * current_state->segments * current_state->segments;
}


/*------------------------------------------------------------*/
void
ogl_stick (vector3 *v1, vector3 *v2, double r1, double r2, colour *c)
{
  assert (v1);
  assert (v2);
  assert (v3_distance (v1, v2) > 0.0);

  start_display_list (FALSE);
  if (c) {
    set_colour_property (c);
  } else {
    set_colour_property (&(current_state->planecolour));
  }
  set_material_properties();
  ogl_cylinder_faces (v1, v2,
		      current_state->stickradius,
		      current_state->segments + 5,
		      FALSE);
  glEndList();
  polygon_count += current_state->segments + 5;
}


/*------------------------------------------------------------*/
void
ogl_strand (void)
{
  int slot;
  colour rgb;
  strand_segment *ss;
  boolean thickness = current_state->strandthickness >= 0.01;

  start_display_list (FALSE);
  if (!thickness) {
    glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glDisable (GL_CULL_FACE);
  }
  set_material_properties();

  if (current_state->colourparts) {

    glBegin (GL_TRIANGLE_STRIP); /* strand face 1, colourparts */
    colour_to_rgb (&(strand_segments[0].c));
    rgb = strand_segments[0].c;
    set_colour_property (&rgb);
    for (slot = 0; slot < strand_segment_count - 3; slot++) {
      ss = strand_segments + slot;
      glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
      glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
      glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
      colour_to_rgb (&(ss->c));
      if (colour_unequal (&rgb, &(ss->c))) {
	glEnd();
	glBegin (GL_TRIANGLE_STRIP);
	rgb = ss->c;
	set_colour_property (&rgb);
	glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
	glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
      }
    }
    glEnd();
    polygon_count += 2 * (strand_segment_count - 3);

    if (thickness) {		/* strand face 2, colourparts */
      glBegin (GL_TRIANGLE_STRIP);
      rgb = strand_segments[0].c;
      set_colour_property (&rgb);
      for (slot = 0; slot < strand_segment_count - 3; slot++) {
	ss = strand_segments + slot;
	glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
	glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
	glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
	if (colour_unequal (&rgb, &(ss->c))) {
	  glEnd();
	  glBegin (GL_TRIANGLE_STRIP);
	  rgb = ss->c;
	  set_colour_property (&rgb);
	  glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
	  glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
	}
      }
      glEnd();
      polygon_count += 2 * (strand_segment_count - 3);
    }

  } else {

    glBegin (GL_TRIANGLE_STRIP); /* strand face 1, single colour */
    set_colour_property (&(current_state->planecolour));
    for (slot = 0; slot < strand_segment_count - 3; slot++) {
      ss = strand_segments + slot;
      glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
      glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
      glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
    }
    glEnd();
    polygon_count += 2 * (strand_segment_count - 3);

    if (thickness) {		/* strand face 2, single colour */
      glBegin (GL_TRIANGLE_STRIP);
      for (slot = 0; slot < strand_segment_count - 3; slot++) {
	ss = strand_segments + slot;
	glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
	glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
	glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
      }
      glEnd();
      polygon_count += 2 * (strand_segment_count - 3);
    }
  }

  ss = strand_segments + strand_segment_count - 3; /* arrow face 1, high */
  glBegin (GL_TRIANGLE_STRIP);
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
  ss--;
  glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
  ss += 2;
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
  ss -= 2;
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
  ss += 2;
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
  ss--;
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
  glEnd();
  polygon_count += 4;

  if (thickness) {
    ss = strand_segments + strand_segment_count - 3; /* arrow face 1, low */
    glBegin (GL_TRIANGLE_STRIP);
    glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    ss--;
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    ss += 2;
    glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    ss -= 2;
    glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    ss += 2;
    glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    ss--;
    glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    glEnd();
    polygon_count += 4;
  }

  if (current_state->colourparts && colour_unequal (&rgb, &(ss->c))) {
    rgb = ss->c;
    set_colour_property (&rgb);
  }

  ss = strand_segments + strand_segment_count - 2; /* arrow face 2, high */
  glBegin (GL_TRIANGLES);
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
  glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
  ss++;
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
  glEnd();
  polygon_count++;;

  if (thickness) {
    ss = strand_segments + strand_segment_count - 2; /* arrow face 2, low */
    glBegin (GL_TRIANGLES);
    glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    ss++;
    glNormal3d (ss->n2.x, ss->n2.y, ss->n2.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    glEnd();
    polygon_count++;;
  }

  if (thickness) {
    vector3 dir1, dir2, normal, normal2;
    strand_segment *ss2;
				/* strand base normal */
    ss = strand_segments;
    v3_difference (&dir1, &(ss->p3), &(ss->p2));
    v3_difference (&dir2, &(ss->p1), &(ss->p2));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);

    if (current_state->colourparts) {

      glBegin (GL_TRIANGLE_STRIP); /* strand base, colourparts */
      rgb = strand_segments[0].c;
      set_colour_property (&rgb);
      glNormal3d (normal.x, normal.y, normal.z);
      glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
      glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
      glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
      glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
      glEnd();
      polygon_count += 2;

      glBegin (GL_TRIANGLE_STRIP); /* strand side 1, colourparts */
      for (slot = 0; slot < strand_segment_count - 3; slot++) {
	ss = strand_segments + slot;
	glNormal3d (ss->n2.x, ss->n2.y, ss->n2.z);
	glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
	glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
	if (colour_unequal (&rgb, &(ss->c))) {
	  glEnd();
	  glBegin (GL_TRIANGLE_STRIP);
	  rgb = ss->c;
	  set_colour_property (&rgb);
	  glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
	  glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
	}
      }
      glEnd();
      polygon_count += 2 * (strand_segment_count - 3);

      glBegin (GL_TRIANGLE_STRIP); /* strand side 2, colourparts */
      rgb = strand_segments[0].c;
      set_colour_property (&rgb);
      for (slot = 0; slot < strand_segment_count - 3; slot++) {
	ss = strand_segments + slot;
	glNormal3d (ss->n4.x, ss->n4.y, ss->n4.z);
	glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
	glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
	if (colour_unequal (&rgb, &(ss->c))) {
	  glEnd();
	  glBegin (GL_TRIANGLE_STRIP);
	  rgb = ss->c;
	  set_colour_property (&rgb);
	  glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
	  glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
	}
      }
      glEnd();
      polygon_count += 2 * (strand_segment_count - 3);

    } else {
      set_colour_property (&(current_state->plane2colour));

      glBegin (GL_TRIANGLE_STRIP); /* strand base, single colour */
      glNormal3d (normal.x, normal.y, normal.z);
      glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
      glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
      glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
      glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
      glEnd();
      polygon_count += 2;

      glBegin (GL_TRIANGLE_STRIP); /* strand side 1, single colour */
      for (slot = 0; slot < strand_segment_count - 3; slot++) {
	ss = strand_segments + slot;
	glNormal3d (ss->n2.x, ss->n2.y, ss->n2.z);
	glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
	glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
      }
      glEnd();
      polygon_count += 2 * (strand_segment_count - 3);

      glBegin (GL_TRIANGLE_STRIP); /* strand side 2, single colour */
      for (slot = 0; slot < strand_segment_count - 3; slot++) {
	ss = strand_segments + slot;
	glNormal3d (ss->n4.x, ss->n4.y, ss->n4.z);
	glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
	glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
      }
      glEnd();
      polygon_count += 2 * (strand_segment_count - 3);
    }
				/* arrow base, either colour */
    ss = strand_segments + strand_segment_count - 3;
    ss2 = strand_segments + strand_segment_count - 4;

    v3_difference (&dir1, &(ss->p3), &(ss->p1));
    v3_difference (&dir2, &(ss->p4), &(ss->p2));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);

    glBegin (GL_TRIANGLES);
    glNormal3d (normal.x, normal.y, normal.z);
    glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    glVertex3d (ss2->p1.x, ss2->p1.y, ss2->p1.z);
    glVertex3d (ss2->p1.x, ss2->p1.y, ss2->p1.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    glVertex3d (ss2->p2.x, ss2->p2.y, ss2->p2.z);
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
    glVertex3d (ss2->p3.x, ss2->p3.y, ss2->p3.z);
    glVertex3d (ss2->p3.x, ss2->p3.y, ss2->p3.z);
    glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
    glVertex3d (ss2->p4.x, ss2->p4.y, ss2->p4.z);
    glEnd();
    polygon_count += 4;
				/* arrow first part 1, either colour */
    ss = strand_segments + strand_segment_count - 2;
    ss2 = ss + 1;

    v3_difference (&dir1, &(ss2->p1), &(ss->p2));
    v3_difference (&dir2, &(ss2->p2), &(ss->p1));
    v3_cross_product (&normal2, &dir1, &dir2);
    v3_normalize (&normal2);

    ss = strand_segments + strand_segment_count - 3;
    ss2 = ss + 1;

    v3_difference (&dir1, &(ss2->p1), &(ss->p2));
    v3_difference (&dir2, &(ss2->p2), &(ss->p1));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);

    glBegin (GL_TRIANGLE_STRIP);
    glNormal3d (normal.x, normal.y, normal.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
    glNormal3d (normal2.x, normal2.y, normal2.z);
    glVertex3d (ss2->p2.x, ss2->p2.y, ss2->p2.z);
    glVertex3d (ss2->p1.x, ss2->p1.y, ss2->p1.z);
    glEnd();
    polygon_count += 2;
				/* arrow last part 1, either colour */
    ss = strand_segments + strand_segment_count - 2;
    ss2 = ss + 1;

    glBegin (GL_TRIANGLE_STRIP);
    if (current_state->colourparts && colour_unequal (&rgb, &(ss->c))) {
      rgb = ss->c;
      set_colour_property (&rgb);
    }
    glNormal3d (normal2.x, normal2.y, normal2.z); /* not quite correct, */
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);    /* but what the hell... */
    glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
    glVertex3d (ss2->p2.x, ss2->p2.y, ss2->p2.z);
    glVertex3d (ss2->p1.x, ss2->p1.y, ss2->p1.z);
    glEnd();
    polygon_count += 2;
				/* arrow first part 2, either colour */
    ss = strand_segments + strand_segment_count - 2;
    ss2 = ss + 1;

    v3_difference (&dir1, &(ss2->p2), &(ss->p4));
    v3_difference (&dir2, &(ss2->p1), &(ss->p3));
    v3_cross_product (&normal2, &dir1, &dir2);
    v3_normalize (&normal2);

    ss = strand_segments + strand_segment_count - 3;
    ss2 = ss + 1;

    v3_difference (&dir1, &(ss2->p3), &(ss->p4));
    v3_difference (&dir2, &(ss2->p4), &(ss->p3));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);

    glBegin (GL_TRIANGLE_STRIP);
    if (current_state->colourparts && colour_unequal (&rgb, &(ss->c))) {
      rgb = ss->c;
      set_colour_property (&rgb);
    }
    glNormal3d (normal.x, normal.y, normal.z);
    glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    glNormal3d (normal2.x, normal2.y, normal2.z);
    glVertex3d (ss2->p4.x, ss2->p4.y, ss2->p4.z);
    glVertex3d (ss2->p3.x, ss2->p3.y, ss2->p3.z);
    glEnd();
    polygon_count += 2;
				/* arrow last part 2, either colour */
    ss = strand_segments + strand_segment_count - 2;
    ss2 = ss + 1;

    glBegin (GL_TRIANGLE_STRIP);
    if (current_state->colourparts && colour_unequal (&rgb, &(ss->c))) {
      rgb = ss->c;
      set_colour_property (&rgb);
    }
    glNormal3d (normal2.x, normal2.y, normal2.z); /* not quite correct, */
    glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);    /* but what the hell... */
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    glVertex3d (ss2->p1.x, ss2->p1.y, ss2->p1.z);
    glVertex3d (ss2->p2.x, ss2->p2.y, ss2->p2.z);
    glEnd();
    polygon_count += 2;

  }

  if (!thickness) {
    glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    glEnable (GL_CULL_FACE);
  }
  glEndList();
}


/*------------------------------------------------------------*/
static boolean current_lighting;
static GLenum current_shademodel;


/*------------------------------------------------------------*/
static void
obj_set_state (boolean lighting, GLenum shademodel)
{
  if (lighting != current_lighting) {
    current_lighting = lighting;
    if (current_lighting) {
      glEnable (GL_LIGHTING);
    } else {
      glDisable (GL_LIGHTING);
    }
  }
  if (shademodel != current_shademodel) {
    current_shademodel = shademodel;
    glShadeModel (current_shademodel);
  }
}


/*------------------------------------------------------------*/
void
ogl_start_object (void)
{
  start_display_list (FALSE);
  glPushAttrib (GL_POINT_BIT | GL_LINE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
  glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  glDisable (GL_CULL_FACE);
  current_lighting = TRUE;
  current_shademodel = GL_SMOOTH;
}


/*------------------------------------------------------------*/
void
ogl_object (int code, vector3 *triplets, int count)
{
  int slot;
  vector3 *v;
  vector3 col, normal;
  double alpha = 1.0 - current_state->transparency;

  assert (triplets);
  assert (count > 0);

  switch (code) {

  case OBJ_POINTS:
    obj_set_state (FALSE, GL_FLAT);
    glPointSize (current_state->linewidth);
    set_colour_property (&(current_state->linecolour));
    glBegin (GL_POINTS);
    for (slot = 0; slot < count; slot++) {
      v = triplets + slot;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    line_point_count += count;
    break;

  case OBJ_POINTS_COLOURS:
    obj_set_state (FALSE, GL_FLAT);
    glPointSize (current_state->linewidth);
    col.x = -1.0;
    glBegin (GL_POINTS);
    for (slot = 1; slot < count; slot += 2) {
      v = triplets + slot;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    line_point_count += count;
    break;

  case OBJ_LINES:
    obj_set_state (FALSE, GL_FLAT);
    glLineWidth ((GLfloat) current_state->linewidth);
    if (current_state->linedash > 0.5) {
      glLineStipple ((GLint) (current_state->linedash + 0.5), 0xAAAA);
      glEnable (GL_LINE_STIPPLE);
    } else {
      glDisable (GL_LINE_STIPPLE);
    }
    set_colour_property (&(current_state->linecolour));
    glBegin (GL_LINE_STRIP);
    for (slot = 0; slot < count; slot++) {
      v = triplets + slot;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    line_point_count += count;
    break;

  case OBJ_LINES_COLOURS:
    obj_set_state (FALSE, GL_SMOOTH);
    col.x = -1.0;
    glLineWidth ((GLfloat) current_state->linewidth);
    if (current_state->linedash > 0.5) {
      glLineStipple ((GLint) (current_state->linedash + 0.5), 0xAAAA);
      glEnable (GL_LINE_STIPPLE);
    } else {
      glDisable (GL_LINE_STIPPLE);
    }
    glBegin (GL_LINE_STRIP);
    for (slot = 1; slot < count; slot += 2) {
      v = triplets + slot;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    line_point_count += count;
    break;

  case OBJ_TRIANGLES:
    obj_set_state (TRUE, GL_FLAT);
    set_colour_property (&(current_state->planecolour));
    glBegin (GL_TRIANGLES);
    for (slot = 0; slot < count; slot += 3) {
      v = triplets + slot;
      v3_triangle_normal (&normal, v, v + 1, v + 2);
      glNormal3d (normal.x, normal.y, normal.z);
      glVertex3d (v->x, v->y, v->z);
      v++;
      glVertex3d (v->x, v->y, v->z);
      v++;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    polygon_count += count / 3;
    break;

  case OBJ_TRIANGLES_COLOURS:
    obj_set_state (TRUE, GL_SMOOTH);
    col.x = -1.0;
    glBegin (GL_TRIANGLES);
    for (slot = 0; slot < count; slot += 6) {
      v = triplets + slot;
      v3_triangle_normal (&normal, v, v + 2, v + 4);
      glNormal3d (normal.x, normal.y, normal.z);
      v++;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glVertex3d (v->x, v->y, v->z);
      v += 3;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glVertex3d (v->x, v->y, v->z);
      v += 3;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    polygon_count += count / 6;
    break;

  case OBJ_TRIANGLES_NORMALS:
    obj_set_state (TRUE, GL_SMOOTH);
    set_colour_property (&(current_state->planecolour));
    glBegin (GL_TRIANGLES);
    for (slot = 1; slot < count; slot += 2) {
      v = triplets + slot;
      glNormal3d (v->x, v->y, v->z);
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    polygon_count += count / 6;
    break;

  case OBJ_TRIANGLES_NORMALS_COLOURS:
    obj_set_state (TRUE, GL_SMOOTH);
    col.x = -1.0;
    glBegin (GL_TRIANGLES);
    for (slot = 2; slot < count; slot += 3) {
      v = triplets + slot;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glNormal3d (v->x, v->y, v->z);
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    polygon_count += count / 9;
    break;

  case OBJ_STRIP:
    obj_set_state (TRUE, GL_FLAT);
    set_colour_property (&(current_state->planecolour));
    glBegin (GL_TRIANGLE_STRIP);
    v = triplets;
    v3_triangle_normal (&normal, v, v + 1, v + 2);
    glNormal3d (normal.x, normal.y, normal.z);
    glVertex3d (v->x, v->y, v->z);
    v++;
    glVertex3d (v->x, v->y, v->z);
    v++;
    glVertex3d (v->x, v->y, v->z);
    for (slot = 3; slot < count; slot++) {
      v = triplets + slot;
      v3_triangle_normal (&normal, v - 2, v - 1, v);
      if (slot % 2) v3_reverse (&normal);
      glNormal3d (normal.x, normal.y, normal.z);
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    polygon_count += count - 2;
    break;

  case OBJ_STRIP_COLOURS:
    obj_set_state (TRUE, GL_SMOOTH);
    col.x = -1.0;
    glBegin (GL_TRIANGLE_STRIP);
    v = triplets + 1;
    if (v3_different (&col, v)) {
      col = *v;
      glColor4d (col.x, col.y, col.z, alpha);
    }
    v--;
    v3_triangle_normal (&normal, v, v + 2, v + 4);
    glNormal3d (normal.x, normal.y, normal.z);
    glVertex3d (v->x, v->y, v->z);
    v += 3;
    if (v3_different (&col, v)) {
      col = *v;
      glColor4d (col.x, col.y, col.z, alpha);
    }
    v--;
    glVertex3d (v->x, v->y, v->z);
    v += 3;
    if (v3_different (&col, v)) {
      col = *v;
      glColor4d (col.x, col.y, col.z, alpha);
    }
    v--;
    glVertex3d (v->x, v->y, v->z);
    for (slot = 6; slot < count; slot += 2) {
      v = triplets + slot + 1;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      v3_triangle_normal (&normal, v - 4, v - 2, v);
      if (slot % 4) v3_reverse (&normal);
      glNormal3d (normal.x, normal.y, normal.z);
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    polygon_count += count / 2 - 2;
    break;

  case OBJ_STRIP_NORMALS:
    obj_set_state (TRUE, GL_SMOOTH);
    set_colour_property (&(current_state->planecolour));
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < count; slot += 2) {
      v = triplets + slot + 1;
      glNormal3d (v->x, v->y, v->z);
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    polygon_count += count / 2 - 2;
    break;

  case OBJ_STRIP_NORMALS_COLOURS:
    obj_set_state (TRUE, GL_SMOOTH);
    col.x = -1.0;
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < count; slot += 3) {
      v = triplets + slot + 2;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glNormal3d (v->x, v->y, v->z);
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    polygon_count += count / 3 - 2;
    break;

  }
}


/*------------------------------------------------------------*/
void
ogl_finish_object (void)
{
  glPopAttrib();
  glEndList();
}


/*------------------------------------------------------------*/
void
ogl_pickable (at3d *atom)
{
  assert (atom);

  if (pickable_array == NULL) {
    pickable_alloc = 1024;
    pickable_array = malloc (pickable_alloc * sizeof (pickable));
    pick_buffer_alloc = 256;
    pick_buffer = malloc (pick_buffer_alloc * sizeof (GLuint));
  } else if (pickable_count >= pickable_alloc) {
    pickable_alloc *= 2;
    pickable_array = realloc (pickable_array,
			      pickable_alloc * sizeof (pickable));
  }

  pickable_array[pickable_count].xyz = atom->xyz;
  pickable_array[pickable_count].molname = str_clone (atom->res->mol->name);
  pickable_array[pickable_count].resname = str_clone (atom->res->name);
  pickable_array[pickable_count].restype = str_clone (atom->res->type);
  pickable_array[pickable_count].atname = str_clone (atom->name);

  pickable_count++;
}
