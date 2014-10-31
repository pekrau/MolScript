/* raster3d.c

   MolScript v2.1.2

   Raster3D; input for the 'render' program.

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
    16-Sep-1997  label output using GLUT stroke character def's
    17-Sep-1997  fairly finished
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "raster3d.h"
#include "global.h"
#include "graphics.h"
#include "segment.h"
#include "state.h"

#ifdef OPENGL_SUPPORT
#include "other/glutstroke.h"
#endif


/*============================================================*/
#define HELIX_RECESS 0.8

#ifdef OPENGL_SUPPORT
extern StrokeFontRec glutStrokeRoman;
#define FONTSCALE_FACTOR 2.75e-5
#define RADIUS_FACTOR 2.4e-4
#define DEFAULT_WINDOW 20.0
#endif

static FILE *proper_outfile;
static FILE *header_file;

static int antialiasing = 3;
static int ntx, nty, npx, npy;
static vector3 lightdirection;

static int triangle_count;
static int sphere_count;
static int cylinder_count;

static colour rgb = { COLOUR_RGB, 0.0, 0.0, 0.0 };

static double material_shininess = 0.2;
static double material_transparency = 0.0;
static colour material_specularcolour = {COLOUR_RGB, 1.0, 1.0, 1.0};
static int special_material = FALSE;


/*------------------------------------------------------------*/
static void
convert_colour (colour *c)
{
  assert (c);

  colour_copy_to_rgb (&rgb, c);
  rgb.x *= rgb.x;		/* Raster3D has a weird concept of RGB */
  rgb.y *= rgb.y;
  rgb.z *= rgb.z;
}


/*------------------------------------------------------------*/
static int
write_triangle (vector3 *p1, vector3 *p2, vector3 *p3)
{
  assert (p1);
  assert (p2);
  assert (p3);

  if ((slab > 0.0) &&
      (((p1->z < -slab) && (p2->z < -slab) && (p3->z < -slab)) ||
       ((p1->z > slab) && (p2->z > slab) && (p3->z > slab)))) return FALSE;

  fprintf (outfile,
	   "1\n%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %g %g %g\n",
	   p1->x, p1->y, p1->z,
	   p2->x, p2->y, p2->z,
	   p3->x, p3->y, p3->z,
	   rgb.x, rgb.y, rgb.z);

  triangle_count++;

  return TRUE;
}


/*------------------------------------------------------------*/
static void
write_normals (vector3 *n1, vector3 *n2, vector3 *n3)
{
  assert (n1);
  assert (n2);
  assert (n3);

  fprintf (outfile,
	   "7\n%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
	   n1->x, n1->y, n1->z,
	   n2->x, n2->y, n2->z,
	   n3->x, n3->y, n3->z);
}


/*------------------------------------------------------------*/
static void
write_vertex_colours (vector3 *c1, vector3 *c2, vector3 *c3)
{
  assert (c1);
  assert (c2);
  assert (c3);

  fprintf (outfile,
	   "17\n%g %g %g %g %g %g %g %g %g\n",
	   c1->x * c1->x, c1->y * c1->y, c1->z * c1->z,
	   c2->x * c2->x, c2->y * c2->y, c2->z * c2->z,
	   c3->x * c3->x, c3->y * c3->y, c3->z * c3->z);
}


/*------------------------------------------------------------*/
static void
write_sphere (vector3 *p, double radius)
{
  assert (p);
  assert (radius > 0.0);

  if ((slab > 0.0) &&
      ((p->z - radius < -slab) || (p->z + radius > slab))) return;

  fprintf (outfile,
	   "2\n%.2f %.2f %.2f %.2f %.3g %.3g %.3g\n",
	   p->x, p->y, p->z, radius, rgb.x, rgb.y, rgb.z);

  sphere_count++;
}


/*------------------------------------------------------------*/
static void
write_cylinder_round (vector3 *p1, vector3 *p2, double radius)
{
  assert (p1);
  assert (p2);
  assert (radius > 0.0);

  if ((slab > 0.0) &&
      (((p1->z - radius < -slab) && (p2->z - radius < -slab)) ||
       (((p1->z + radius > slab) && (p2->z + radius > slab))))) return;

  fprintf (outfile,
	   "3\n%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %g %g %g\n",
	   p1->x, p1->y, p1->z, radius,
	   p2->x, p2->y, p2->z, radius,
	   rgb.x, rgb.y, rgb.z);

  cylinder_count++;
}


/*------------------------------------------------------------*/
static void
write_cylinder_flat (vector3 *p1, vector3 *p2, double radius)
{
  assert (p1);
  assert (p2);
  assert (radius > 0.0);

  if ((slab > 0.0) &&
      (((p1->z - radius < -slab) && (p2->z - radius < -slab)) ||
       (((p1->z + radius > slab) && (p2->z + radius > slab))))) return;

  fprintf (outfile,
	   "5\n%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %g %g %g\n",
	   p1->x, p1->y, p1->z, radius,
	   p2->x, p2->y, p2->z, radius,
	   rgb.x, rgb.y, rgb.z);

  cylinder_count++;
}


/*------------------------------------------------------------*/
static void
write_material (void)
{
  if ((current_state->shininess == material_shininess) &&
      (current_state->transparency == material_transparency) &&
      ! colour_unequal (&(current_state->specularcolour),
			&(material_specularcolour))) return;

  if (special_material) fprintf (outfile, "9\n");

  material_shininess = current_state->shininess;
  material_transparency = current_state->transparency;
  material_specularcolour = current_state->specularcolour;

  convert_colour (&(material_specularcolour));
  fprintf (outfile, "8\n%.2g %.3g %.3g %.3g %.3g %.3g 1 0 0 0\n",
	   material_shininess * 128.0,
	   (material_transparency == 0.0) ? 0.25 : 0.6,
	   rgb.x, rgb.y, rgb.z,
	   material_transparency);

  special_material = TRUE;
}


/*------------------------------------------------------------*/
static void
write_line (vector3 *p1, vector3 *p2)
{
  double radius = LINEWIDTH_FACTOR * current_state->linewidth;

  assert (p1);
  assert (p2);

  if (radius < LINEWIDTH_MINIMUM) radius = LINEWIDTH_MINIMUM;

  if (current_state->linedash == 0.0) {
    write_cylinder_round (p1, p2, radius);

  } else {
    int slot, parts;
    vector3 pos1, pos2;

    parts = (int) (v3_distance (p1, p2) * 16.0 / current_state->linedash);
    if (parts <= 1) {
      write_cylinder_round (p1, p2, radius);

    } else if ((current_state->linewidth != 0.0) &&
	       (current_state->linedash / current_state->linewidth < 2.0)) {
      parts /= 2;
      for (slot= 0; slot <= parts; slot++) {
	v3_between (&pos1, p1, p2, (double) slot / (double) parts);
	write_sphere (&pos1, radius);
      }

    } else {
      if (parts % 2 == 0) parts++; /* last segment should end on last point */
      write_sphere (p1, radius);
      for (slot = 0; slot < parts; slot += 2) {
	v3_between (&pos1, p1, p2, (double) slot / (double) parts);
	v3_between (&pos2, p1, p2, (double) (slot + 1) / (double) parts);
	write_cylinder_flat (&pos1, &pos2, radius);
      }
      write_sphere (p2, radius);
    }
  }
}


/*------------------------------------------------------------*/
void
r3d_set (void)
{
  output_first_plot = r3d_first_plot;
  output_start_plot = r3d_start_plot;
  output_finish_plot = r3d_finish_plot;
  output_finish_output = r3d_finish_output;

  set_area = r3d_set_area;
  set_background = r3d_set_background;
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
  output_directionallight = r3d_directionallight;
  output_pointlight = r3d_pointlight;
  output_spotlight = do_nothing;
  output_comment = r3d_comment;

  output_coil = r3d_coil;
  output_cylinder = r3d_cylinder;
  output_helix = r3d_helix;
  output_label = r3d_label;
  output_line = r3d_line;
  output_sphere = r3d_sphere;
  output_stick = r3d_stick;
  output_strand = r3d_strand;

  output_start_object = do_nothing;
  output_object = r3d_object;
  output_finish_object = do_nothing;

  output_pickable = NULL;

  constant_colours_to_rgb();

  output_mode = RASTER3D_MODE;
}


/*------------------------------------------------------------*/
void
r3d_first_plot (void)
{
  if (!first_plot)
    yyerror ("only one plot per input file allowed for Raster3D");

  set_outfile ("w");

  header_file = fopen ("header.r3d", "r");
  if (header_file) {
    char str[256];

    if (fgets (str, 256, header_file) == NULL)
      yyerror ("could not read from the 'header.r3d' file");
    if (strlen (str) == 255) str[254] = '\n';
    if (fprintf (outfile, "%s", str) < 0) goto output_error;

  } else {
    if (fprintf (outfile, "render (Raster3D) input file, %s, %s\n",
		 program_str, copyright_str) < 0) goto output_error;
  }

  proper_outfile = outfile;
  if (tmp_filename == NULL) {
    outfile = tmpfile();
  } else {
    outfile = fopen (tmp_filename, "w+");
  }
  if (outfile == NULL) yyerror ("could not create the temporary output file");

  switch (antialiasing) {
  case 1:
    npx = 10;
    ntx = (output_width - 1) / npx + 1;
    npy = 10;
    nty = (output_height - 1) / npy + 1;
    break;
  case 2:
    npx = 20;
    ntx = (2 * output_width - 1) / npx + 1;
    npy = 20;
    nty = (2 * output_height - 1) / npy + 1;
    break;
  case 3:
    npx = 15;
    ntx = (output_width + (output_width - 1) / 2 - 1) / npx + 1;
    npy = 15;
    nty = (output_height + (output_height - 1) / 2 - 1) / npy + 1;
    break;
  default:
    assert (FALSE);
    break;
  }

  v3_initialize (&lightdirection, 1.0, 1.0, 1.0);
  return;

output_error:
  yyerror ("could not write to the output Raster3D file");
}


/*------------------------------------------------------------*/
void
r3d_finish_output (void)
{
  int c;
  FILE *tmp_outfile;

  tmp_outfile = outfile;
  outfile = proper_outfile;

  if (header_file) {
    int slot;
    char str[256];

    for (slot = 0; slot < 14; slot++) {
      if (fgets (str, 256, header_file) == NULL)
	yyerror ("reading from the 'header.r3d' file");
      if (strlen (str) == 255) {
	while (((c = fgetc (header_file)) != '\n') && (c != EOF)) ;
	str[254] = '\n';
      }
      fprintf (outfile, "%s", str);
    }

  } else {
    fprintf (outfile, "%i %i      tiles in x,y\n", ntx, nty);
    fprintf (outfile, "%i %i      pixels (x,y) per tile\n", npx, npy);
    fprintf (outfile, "%i          antialiasing level (1,2,3)\n",antialiasing);
    convert_colour (&background_colour);
    fprintf (outfile, "%.3g %.3g %.3g      background colour\n",
	              rgb.x, rgb.y, rgb.z);
    if (shadows) {
      PRINT ("T          shadows cast (F = no shadows cast)\n");
    } else {
      PRINT ("F          no shadows cast (T = shadows cast)\n");
    }
    PRINT ("25.6       Phong power (specular highlights)\n");
    if (headlight) {
      PRINT ("0.25       secondary light contribution\n");
    } else {
      PRINT ("0          no secondary light contribution\n");
    }
    PRINT ("0.05       ambient light contribution\n");
    PRINT ("0.25       specular reflection component\n");
    PRINT ("0          eye position (0 = no perspective)\n");
    fprintf (outfile, "%g %g %g      main light source position\n",
	     lightdirection.x, lightdirection.y, lightdirection.z);
    PRINT ("1 0 0 0    view matrix: input coordinate transformation\n");
    PRINT ("0 1 0 0\n");
    PRINT ("0 0 1 0\n");
  }

  fprintf (outfile, "0 0 0 %g\n", 2.0 * window);
  PRINT ("3          mixed objects\n");
  PRINT ("*          (free format triangle and plane descriptors)\n");
  PRINT ("*          (free format sphere descriptors)\n");
  PRINT ("*          (free format cylinder descriptors)\n");

  if (header_file) fclose (header_file);

  fseek (tmp_outfile, 0L, SEEK_SET);
  while ((c = fgetc (tmp_outfile)) != EOF) fputc (c, outfile);
  fclose (tmp_outfile);
  if (tmp_filename != NULL) remove (tmp_filename);
}


/*------------------------------------------------------------*/
void
r3d_start_plot (void)
{
  triangle_count = 0;
  sphere_count = 0;
  cylinder_count = 0;

  set_area_values (-0.5, -0.5, 0.5, 0.5);
  background_colour = black_colour;
}


/*------------------------------------------------------------*/
void
r3d_finish_plot (void)
{
  set_extent();

  if (message_mode)
    fprintf (stderr, "%i triangles, %i spheres and %i cylinders.\n",
	     triangle_count, sphere_count, cylinder_count);
}


/*------------------------------------------------------------*/
void
r3d_set_area (void)
{
  if (message_mode) fprintf (stderr, "ignoring 'area' for Raster3D output\n");
  clear_dstack();
}


/*------------------------------------------------------------*/
void
r3d_set_background (void)
{
  background_colour = given_colour;
}


/*------------------------------------------------------------*/
void
r3d_directionallight (void)
{
  assert ((dstack_size == 3) || (dstack_size == 6));

  if (dstack_size == 3) {
    lightdirection.x = dstack[0];
    lightdirection.y = dstack[1];
    lightdirection.z = dstack[2];
  } else {
    lightdirection.x = dstack[3] - dstack[0];
    lightdirection.y = dstack[4] - dstack[1];
    lightdirection.z = dstack[5] - dstack[2];
  }
  v3_normalize (&lightdirection);
  clear_dstack();
}


/*------------------------------------------------------------*/
void
r3d_pointlight (void)
{
  assert (dstack_size == 3);

  convert_colour (&(current_state->lightcolour));
  fprintf (outfile, "13\n%.2f %.2f %.2f %.2f 0.25 1 25.6 %g %g %g\n",
	   dstack[0], dstack[1], dstack[2], current_state->lightradius,
	   rgb.x, rgb.y, rgb.z);
  clear_dstack();
}


/*------------------------------------------------------------*/
void
r3d_comment (char *str)
{
  assert (str);

  fprintf (outfile, "# %s\n", str);
}


/*------------------------------------------------------------*/
void
r3d_coil (void)
{
  int slot;
  coil_segment *cs1, *cs2;

  write_material();

  if (current_state->colourparts) {

    for (slot = 1; slot < coil_segment_count; slot++) {
      cs1 = coil_segments + slot - 1;
      cs2 = coil_segments + slot;
      convert_colour (&(cs1->c));
      write_cylinder_round (&(cs1->p), &(cs2->p), current_state->coilradius);
    }

  } else {
    convert_colour (&(current_state->planecolour));

    for (slot = 1; slot < coil_segment_count; slot++) {
      cs1 = coil_segments + slot - 1;
      cs2 = coil_segments + slot;
      write_cylinder_round (&(cs1->p), &(cs2->p), current_state->coilradius);
    }
  }

  cylinder_count += coil_segment_count;
}


/*------------------------------------------------------------*/
void
r3d_cylinder (vector3 *v1, vector3 *v2)
{
  assert (v1);
  assert (v2);
  assert (v3_distance (v1, v2) > 0.0);

  write_material();
  convert_colour (&(current_state->planecolour));
  write_cylinder_flat (v1, v2, current_state->cylinderradius);
}


/*------------------------------------------------------------*/
void
r3d_helix (void)
{
  int slot;
  helix_segment *hs1, *hs2;
  vector3 p1, p2, p21, p22;
  double radius = current_state->helixthickness / 2.0;
  double offset;

  write_material();
  convert_colour (&(current_state->planecolour));

  for (slot = 1; slot < helix_segment_count; slot++) { /* outer flat side */
    hs1 = helix_segments + slot - 1;
    hs2 = hs1 + 1;
    if (current_state->colourparts) convert_colour (&(hs1->c));
    if (write_triangle (&(hs1->p1), &(hs1->p2), &(hs2->p1))) 
      write_normals (&(hs1->n), &(hs1->n), &(hs2->n));
    if (write_triangle (&(hs1->p2), &(hs2->p1), &(hs2->p2)))
      write_normals (&(hs1->n), &(hs2->n), &(hs2->n));
  }

  if (current_state->helixthickness > 0.01) {

    offset = - HELIX_RECESS * radius;

    hs1 = helix_segments;	/* first segment edge */
    if (current_state->colourparts) convert_colour (&(hs1->c));
    v3_sum_scaled (&p1, &(hs1->p1), offset, &(hs1->n));
    v3_sum_scaled (&p2, &(hs1->p2), offset, &(hs1->n));
    write_cylinder_round (&p1, &p2, radius);

    for (slot = 1; slot < helix_segment_count; slot++) { /* top and  */
      hs1 = helix_segments + slot - 1;                   /* bottom edges */
      hs2 = hs1 + 1;
      if (current_state->colourparts) convert_colour (&(hs1->c));
      v3_sum_scaled (&p1, &(hs1->p1), offset, &(hs1->n));
      v3_sum_scaled (&p2, &(hs2->p1), offset, &(hs2->n));
      write_cylinder_round (&p1, &p2, radius);
      v3_sum_scaled (&p1, &(hs1->p2), offset, &(hs1->n));
      v3_sum_scaled (&p2, &(hs2->p2), offset, &(hs2->n));
      write_cylinder_round (&p1, &p2, radius);
    }

    hs1 = helix_segments + helix_segment_count - 1; /* last segment edge */
    if (current_state->colourparts) convert_colour (&(hs1->c));
    v3_sum_scaled (&p1, &(hs1->p1), offset, &(hs1->n));
    v3_sum_scaled (&p2, &(hs1->p2), offset, &(hs1->n));
    write_cylinder_round (&p1, &p2, radius);
  }

  if (current_state->helixthickness > 0.01) {
    offset = (HELIX_RECESS + 1.0) / 2.0 * current_state->helixthickness;
  } else {
    offset = 0.02;
  }

  convert_colour (&(current_state->plane2colour));

  for (slot = 0; slot < helix_segment_count; slot++) {
    v3_reverse (&(helix_segments + slot)->n);
  }

  for (slot = 1; slot < helix_segment_count; slot++) { /* inner flat side */
    hs1 = helix_segments + slot - 1;
    hs2 = hs1 + 1;
    if (current_state->colourparts) convert_colour (&(hs1->c));
    v3_sum_scaled (&p1, &(hs1->p1), offset, &(hs1->n));
    v3_sum_scaled (&p2, &(hs1->p2), offset, &(hs1->n));
    v3_sum_scaled (&p21, &(hs2->p1), offset, &(hs2->n));
    v3_sum_scaled (&p22, &(hs2->p2), offset, &(hs2->n));
    if (write_triangle (&p1, &p2, &p21))
      write_normals (&(hs1->n), &(hs1->n), &(hs2->n));
    if (write_triangle (&p2, &p21, &p22))
      write_normals (&(hs1->n), &(hs2->n), &(hs2->n));
  }
}


/*------------------------------------------------------------*/
void
r3d_label (vector3 *p, char *label, colour *c)
{
#ifdef OPENGL_SUPPORT
  const StrokeCharRec *ch;
  const StrokeRec *stroke;
  const CoordRec *coord;
  StrokeFontPtr fontinfo = &glutStrokeRoman;
  int i, j;
  double offset = 0.0;
  vector3 pos1, pos2;
  double font_scale, radius;
  double spacing = 10.0;
#endif

  assert (p);
  assert (label);
  assert (*label);

#ifdef OPENGL_SUPPORT

  if (window > 0.0) {
    font_scale = FONTSCALE_FACTOR * window * current_state->labelsize;
    radius = RADIUS_FACTOR * window * current_state->labelsize;
  } else {
    font_scale = FONTSCALE_FACTOR * DEFAULT_WINDOW * current_state->labelsize;
    radius = RADIUS_FACTOR * DEFAULT_WINDOW * current_state->labelsize;
  }

  write_material();
  if (c) {
    convert_colour (c);
  } else {
    convert_colour (&(current_state->linecolour));
  }

  for (; *label; label++) {
    if (*label >= fontinfo->num_chars) continue;

    ch = &(fontinfo->ch[*label]);
    if (ch) {
      for (i = ch->num_strokes, stroke = ch->stroke; i > 0; i--, stroke++) {
	coord = stroke->coord;
	v3_initialize (&pos1, coord->x, coord->y, 0.0);
	v3_scale (&pos1, font_scale);
	v3_add (&pos1, p);
	v3_add (&pos1, &(current_state->labeloffset));
	pos1.x += offset;
	coord++;
	for (j = stroke->num_coords - 1; j > 0; j--, coord++) {
	  v3_initialize (&pos2, coord->x, coord->y, 0.0);
	  v3_scale (&pos2, font_scale);
	  v3_add (&pos2, p);
	  v3_add (&pos2, &(current_state->labeloffset));
	  pos2.x += offset;
	  write_cylinder_round (&pos1, &pos2, radius);
	  pos1 = pos2;
	}
      }
      offset += font_scale * (ch->right + spacing);
    }
  }

#else
  not_implemented ("r3d_label");
#endif
}


/*------------------------------------------------------------*/
void
r3d_line (boolean polylines)
{
  int slot;
  line_segment *ls1, *ls2;

  if (line_segment_count < 2) return;

  write_material();

  if (current_state->colourparts) {
    
    if (polylines) {

      for (slot = 1; slot < line_segment_count; slot++) {
	ls1 = line_segments + slot - 1;
	ls2 = line_segments + slot;
	if (ls2->new) continue;
	convert_colour (&(ls1->c));
	write_line (&(ls1->p), &(ls2->p));
      }

    } else {
      for (slot = 0; slot < line_segment_count; slot += 2) {
	ls1 = line_segments + slot;
	ls2 = line_segments + slot + 1;
	convert_colour (&(ls1->c));
	write_line (&(ls1->p), &(ls2->p));
      }
    }

  } else {
    convert_colour (&(line_segments[0].c));

    if (polylines) {
      for (slot = 1; slot < line_segment_count; slot++) {
	ls1 = line_segments + slot - 1;
	ls2 = line_segments + slot;
	if (ls2->new) continue;
	write_line (&(ls1->p), &(ls2->p));
      }

    } else {
      for (slot = 0; slot < line_segment_count; slot += 2) {
	ls1 = line_segments + slot;
	ls2 = line_segments + slot + 1;
	write_line (&(ls1->p), &(ls2->p));
      }
    }
  }

}


/*------------------------------------------------------------*/
void
r3d_sphere (at3d *at, double radius)
{
  assert (at);
  assert (radius > 0.0);

  write_material();
  convert_colour (&(at->colour));
  write_sphere (&(at->xyz), radius);
}


/*------------------------------------------------------------*/
void
r3d_stick (vector3 *v1, vector3 *v2, double r1, double r2, colour *c)
{
  assert (v1);
  assert (v2);
  assert (v3_distance (v1, v2) > 0.0);

  write_material();
  if (c) {
    convert_colour (c);
  } else {
    convert_colour (&(current_state->planecolour));
  }
  write_cylinder_round (v1, v2, current_state->stickradius);
}


/*------------------------------------------------------------*/
void
r3d_strand (void)
{
  int slot;
  int thickness = current_state->strandthickness >= 0.01;
  strand_segment *ss1, *ss2;

  write_material();
				/* strand face 1 */
  if (! current_state->colourparts)
    convert_colour (&(current_state->planecolour));

  for (slot = 1; slot < strand_segment_count - 3; slot++) {
    ss1 = strand_segments + slot - 1;
    ss2 = ss1 + 1;
    if (current_state->colourparts) convert_colour (&(ss1->c));
    if (write_triangle (&(ss1->p1), &(ss1->p4), &(ss2->p1)))
      write_normals (&(ss1->n1), &(ss1->n1), &(ss2->n1));
    if (write_triangle (&(ss1->p4), &(ss2->p1), &(ss2->p4)))
      write_normals (&(ss1->n1), &(ss2->n1), &(ss2->n1));
  }
				/* arrow face 1, high */
  ss1 = strand_segments + strand_segment_count - 3;
  ss2 = ss1 + 1;
  if (write_triangle (&(ss1->p1), &(ss1->p4), &(ss2->p1)))
    write_normals (&(ss1->n1), &(ss1->n1), &(ss2->n1));
  if (write_triangle (&(ss1->p4), &(ss2->p1), &(ss2->p4)))
    write_normals (&(ss1->n1), &(ss2->n1), &(ss2->n1));

				/* arrow face 2, high */
  ss1 = strand_segments + strand_segment_count - 2;
  ss2 = ss1 + 1;
  if (current_state->colourparts) convert_colour (&(ss1->c));
				/* better when *not* writing these normals */
  write_triangle (&(ss1->p1), &(ss1->p4), &(ss2->p1));

  if (thickness) {
				/* strand face 2 */
    for (slot = 1; slot < strand_segment_count - 3; slot++) {
      ss1 = strand_segments + slot - 1;
      ss2 = ss1 + 1;
      if (current_state->colourparts) convert_colour (&(ss1->c));
      if (write_triangle (&(ss1->p2), &(ss1->p3), &(ss2->p2)))
	write_normals (&(ss1->n3), &(ss1->n3), &(ss2->n3));
      if (write_triangle (&(ss1->p3), &(ss2->p2), &(ss2->p3)))
	write_normals (&(ss1->n3), &(ss2->n3), &(ss2->n3));
    }
				/* arrow face 1, low */
    ss1 = strand_segments + strand_segment_count - 3;
    ss2 = ss1 + 1;
    if (write_triangle (&(ss1->p2), &(ss1->p3), &(ss2->p2)))
      write_normals (&(ss1->n3), &(ss1->n3), &(ss2->n3));
    if (write_triangle (&(ss1->p3), &(ss2->p2), &(ss2->p3)))
      write_normals (&(ss1->n3), &(ss2->n3), &(ss2->n3));
				/* arrow face 2, low */
    ss1 = strand_segments + strand_segment_count - 2;
    ss2 = ss1 + 1;
    if (current_state->colourparts) convert_colour (&(ss1->c));
				/* better when *not* writing these normals */
    write_triangle (&(ss1->p2), &(ss1->p3), &(ss2->p2));

				/* strand side 1 */
    if (!current_state->colourparts)
      convert_colour (&(current_state->plane2colour));

    for (slot = 1; slot < strand_segment_count - 3; slot++) {
      ss1 = strand_segments + slot - 1;
      ss2 = ss1 + 1;
      if (current_state->colourparts) convert_colour (&(ss1->c));
      write_triangle (&(ss1->p2), &(ss1->p1), &(ss2->p2));
      write_triangle (&(ss1->p1), &(ss2->p2), &(ss2->p1));
    }
				/* strand side 2 */
    for (slot = 1; slot < strand_segment_count - 3; slot++) {
      ss1 = strand_segments + slot - 1;
      ss2 = ss1 + 1;
      if (current_state->colourparts) convert_colour (&(ss1->c));
      write_triangle (&(ss1->p3), &(ss1->p4), &(ss2->p3));
      write_triangle (&(ss1->p4), &(ss2->p3), &(ss2->p4));
    }
				/* arrow base */
    ss1 = strand_segments + strand_segment_count - 3;
    ss2 = ss1 - 1;
    if (current_state->colourparts) convert_colour (&(ss2->c));
    write_triangle (&(ss1->p1), &(ss1->p2), &(ss2->p1));
    write_triangle (&(ss2->p1), &(ss1->p2), &(ss2->p2));
    write_triangle (&(ss1->p3), &(ss1->p4), &(ss2->p3));
    write_triangle (&(ss2->p3), &(ss1->p4), &(ss2->p4));

				/* strand base */
				/* This peculiar order is used to avoid */
				/* spill-over of normal vectors, due to */
    ss1 = strand_segments;	/* render's smooth-ribbon algorithm */
    if (current_state->colourparts) convert_colour (&(ss1->c));
    write_triangle (&(ss1->p1), &(ss1->p2), &(ss1->p3));
    write_triangle (&(ss1->p3), &(ss1->p4), &(ss1->p1));

				/* arrow side 1 */
    ss1 = strand_segments + strand_segment_count - 3;
    ss2 = ss1 + 1;
    if (current_state->colourparts) convert_colour (&(ss1->c));
    write_triangle (&(ss1->p2), &(ss1->p1), &(ss2->p2));
    write_triangle (&(ss1->p1), &(ss2->p2), &(ss2->p1));
    ss1 = ss2;
    ss2++;
    if (current_state->colourparts) convert_colour (&(ss1->c));
    write_triangle (&(ss1->p2), &(ss1->p1), &(ss2->p2));
    write_triangle (&(ss1->p1), &(ss2->p2), &(ss2->p1));

				/* arrow side 2 */
    ss1 = strand_segments + strand_segment_count - 3;
    ss2 = ss1 + 1;
    if (current_state->colourparts) convert_colour (&(ss1->c));
    write_triangle (&(ss1->p3), &(ss1->p4), &(ss2->p3));
    write_triangle (&(ss1->p4), &(ss2->p3), &(ss2->p4));
    ss1 = ss2;
    ss2++;
    if (current_state->colourparts) convert_colour (&(ss1->c));
    write_triangle (&(ss1->p3), &(ss1->p4), &(ss2->p2));
    write_triangle (&(ss1->p4), &(ss2->p2), &(ss2->p1));
  }
}


/*------------------------------------------------------------*/
static void
convert_vector_colour (vector3 *v)
{
  assert (v);

  rgb.x = v->x * v->x;		/* Raster3D has a weird concept of RGB */
  rgb.y = v->y * v->y;
  rgb.z = v->z * v->z;
}


/*------------------------------------------------------------*/
void
r3d_object (int code, vector3 *triplets, int count)
{
  int slot;
  vector3 *v;
  vector3 normal;
  double radius;

  assert (triplets);
  assert (count > 0);

  write_material();

  switch (code) {

  case OBJ_POINTS:
    radius = LINEWIDTH_FACTOR * current_state->linewidth;
    if (radius < LINEWIDTH_MINIMUM) radius = LINEWIDTH_MINIMUM;
    convert_colour (&(current_state->linecolour));
    for (slot = 0; slot < count; slot++) {
      write_sphere (triplets + slot, radius);
    }
    break;

  case OBJ_POINTS_COLOURS:
    radius = LINEWIDTH_FACTOR * current_state->linewidth;
    if (radius < LINEWIDTH_MINIMUM) radius = LINEWIDTH_MINIMUM;
    for (slot = 0; slot < count; slot += 2) {
      convert_vector_colour (triplets + slot + 1);
      write_sphere (triplets + slot, radius);
    }
    break;

  case OBJ_LINES:
    convert_colour (&(current_state->linecolour));
    for (slot = 1; slot < count; slot++) {
      write_line (triplets + slot - 1, triplets + slot);
    }
    break;

  case OBJ_LINES_COLOURS:
    for (slot = 2; slot < count; slot += 2) {
      v = triplets + slot;
      convert_vector_colour (v - 1);
      write_line (v - 2, v);
    }
    break;

  case OBJ_TRIANGLES:
    convert_colour (&(current_state->planecolour));
    for (slot = 0; slot < count; slot += 3) {
      v = triplets + slot;
      write_triangle (v, v + 1, v + 2);
    }
    break;

  case OBJ_TRIANGLES_COLOURS:
    for (slot = 0; slot < count; slot += 6) {
      v = triplets + slot;
      convert_vector_colour (v + 1);
      if (write_triangle (v, v + 2, v + 4)) {
	write_vertex_colours (v + 1, v + 3, v + 5);
      }
    }
    break;

  case OBJ_TRIANGLES_NORMALS:
    convert_colour (&(current_state->planecolour));
    for (slot = 0; slot < count; slot += 6) {
      v = triplets + slot;
      if (write_triangle (v, v + 2, v + 4)) {
	write_normals (v + 1, v + 3, v + 5);
      }
    }
    break;

  case OBJ_TRIANGLES_NORMALS_COLOURS:
    for (slot = 0; slot < count; slot += 9) {
      v = triplets + slot;
      convert_vector_colour (v + 2);
      if (write_triangle (v, v + 3, v + 6)) {
	write_normals (v + 1, v + 4, v + 7);
	write_vertex_colours (v + 2, v + 5, v + 8);
      }
    }
    break;

  case OBJ_STRIP:
    convert_colour (&(current_state->planecolour));
    for (slot = 2; slot < count; slot++) {
      v = triplets + slot;
      if (write_triangle (v - 2, v - 1, v)) {
	v3_triangle_normal (&normal, v - 2, v - 1, v);
	write_normals (&normal, &normal, &normal);
      }
    }
    break;

  case OBJ_STRIP_COLOURS:
    for (slot = 4; slot < count; slot += 2) {
      v = triplets + slot;
      convert_vector_colour (v + 1);
      if (write_triangle (v - 4, v - 2, v)) {
	v3_triangle_normal (&normal, v - 4, v - 2, v);
	write_normals (&normal, &normal, &normal);
	write_vertex_colours (v - 3, v - 1, v + 1);
      }
    }
    break;

  case OBJ_STRIP_NORMALS:
    convert_colour (&(current_state->planecolour));
    for (slot = 4; slot < count; slot += 2) {
      v = triplets + slot;
      if (write_triangle (v - 4, v - 2, v)) {
	write_normals (v - 3, v - 1, v + 1);
      }
    }
    break;

  case OBJ_STRIP_NORMALS_COLOURS:
    for (slot = 6; slot < count; slot += 3) {
      v = triplets + slot;
      convert_vector_colour (v + 2);
      if (write_triangle (v - 6, v - 3, v)) {
	write_normals (v - 5, v - 2, v + 1);
	write_vertex_colours (v - 4, v - 1, v + 2);
      }
    }
    break;

  }
}


/*------------------------------------------------------------*/
void
r3d_set_antialiasing (int new)
{
  assert (new >= 1);
  assert (new <= 3);

  antialiasing = new;
}
