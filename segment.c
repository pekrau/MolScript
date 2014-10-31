/* segment.c

   MolScript v2.1.2

   Segment data structure and routines.

   Copyright (C) 1997-1998 Per Kraulis
    18-Aug-1997  split out of vrml.c; generalized
*/

#include <stdlib.h>

#include "segment.h"


/*------------------------------------------------------------*/
line_segment *line_segments = NULL;
int line_segment_count;
static int line_segment_alloc;

strand_segment *strand_segments = NULL;
int strand_segment_count;
static int strand_segment_alloc;

helix_segment *helix_segments = NULL;
int helix_segment_count;
static int helix_segment_alloc;

coil_segment *coil_segments = NULL;
int coil_segment_count;
static int coil_segment_alloc;


/*------------------------------------------------------------*/
void
line_segment_init (void)
{
  if (line_segments == NULL) {
    line_segment_alloc = 512;
    line_segments = malloc (line_segment_alloc * sizeof (line_segment));
  }
  line_segment_count = 0;
}


/*------------------------------------------------------------*/
line_segment *
line_segment_next (void)
{
  line_segment *new;

  if (line_segment_count >= line_segment_alloc) {
    line_segment_alloc *= 2;
    line_segments = realloc (line_segments,
			     line_segment_alloc * sizeof (line_segment));
  }
  new = &(line_segments[line_segment_count++]);
  new->new = FALSE;
  return new;
}


/*------------------------------------------------------------*/
void
strand_segment_init (void)
{
  if (strand_segments == NULL) {
    strand_segment_alloc = 256;
    strand_segments = malloc (strand_segment_alloc * sizeof (strand_segment));
  }
  strand_segment_count = 0;
}


/*------------------------------------------------------------*/
strand_segment *
strand_segment_next (void)
{
  if (strand_segment_count >= strand_segment_alloc) {
    strand_segment_alloc *= 2;
    strand_segments = realloc (strand_segments,
			       strand_segment_alloc * sizeof (strand_segment));
  }
  return &(strand_segments[strand_segment_count++]);
}


/*------------------------------------------------------------*/
void
helix_segment_init (void)
{
  if (helix_segments == NULL) {
    helix_segment_alloc = 512;
    helix_segments = malloc (helix_segment_alloc * sizeof (helix_segment));
  }
  helix_segment_count = 0;
}


/*------------------------------------------------------------*/
helix_segment *
helix_segment_next (void)
{
  if (helix_segment_count >= helix_segment_alloc) {
    helix_segment_alloc *= 2;
    helix_segments = realloc (helix_segments,
			      helix_segment_alloc * sizeof (helix_segment));
  }
  return &(helix_segments[helix_segment_count++]);
}


/*------------------------------------------------------------*/
void
coil_segment_init (void)
{
  if (coil_segments == NULL) {
    coil_segment_alloc = 512;
    coil_segments = malloc (coil_segment_alloc * sizeof (coil_segment));
  }
  coil_segment_count = 0;
}


/*------------------------------------------------------------*/
coil_segment *
coil_segment_next (void)
{
  if (coil_segment_count >= coil_segment_alloc) {
    coil_segment_alloc *= 2;
    coil_segments = realloc (coil_segments,
			     coil_segment_alloc * sizeof (coil_segment));
  }
  return &(coil_segments[coil_segment_count++]);
}
