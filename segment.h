/* segment.h

   MolScript v2.1.2

   Segment data structure and routines.

   Copyright (C) 1997-1998 Per Kraulis
    18-Aug-1997  split out of vrml.c; generalized
*/

#ifndef SEGMENT_H
#define SEGMENT_H 1

#include "clib/vector3.h"
#include "clib/colour.h"

typedef struct {
  vector3 p;
  colour c;
  boolean new;
} line_segment;

extern line_segment *line_segments;
extern int line_segment_count;

void line_segment_init (void);
line_segment *line_segment_next (void);

typedef struct {
  vector3 p1, p2, p3, p4;
  vector3 n1, n2, n3, n4;
  colour c;
} strand_segment;

extern strand_segment *strand_segments;
extern int strand_segment_count;

void strand_segment_init (void);
strand_segment *strand_segment_next (void);

typedef struct {
  vector3 p1, p2;
  vector3 a, n;
  colour c;
} helix_segment;

extern helix_segment *helix_segments;
extern int helix_segment_count;

void helix_segment_init (void);
helix_segment *helix_segment_next (void);

typedef struct {
  vector3 p, p1, p2, p3, p4;
  vector3 n1, n2, n3, n4;
  colour c;
} coil_segment;

extern coil_segment *coil_segments;
extern int coil_segment_count;

void coil_segment_init (void);
coil_segment *coil_segment_next (void);

#endif
