/* state.h

   MolScript v2.1.2

   Graphics state.

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
     2-Jan-1997  largely finished
    26-Apr-1998  push and pop implemented
*/

#ifndef STATE_H
#define STATE_H 1

#include "clib/vector3.h"

#include "col.h"

typedef struct s_state state;

struct s_state {
  state   *prev;
  double  bonddistance;
  double  bondcross;
  double  coilradius;
  boolean colourparts;
  double  cylinderradius;
  double  depthcue;
  colour  emissivecolour;
  double  helixthickness;
  double  helixwidth;
  boolean hsbramp;
  boolean hsbrampreverse;
  double  labelbackground;
  boolean labelcentre;
  boolean labelclip;
  int     *labelmask;
  int     labelmasklength;
  vector3 labeloffset;
  boolean labelrotation;
  double  labelsize;
  double  lightambientintensity;
  vector3 lightattenuation;
  colour  lightcolour;
  double  lightintensity;
  double  lightradius;
  colour  linecolour;
  double  linedash;
  double  linewidth;
  boolean objecttransform;
  colour  planecolour;
  colour  plane2colour;
  boolean regularexpression;
  int     segments;
  double  segmentsize;
  double  shading;
  double  shadingexponent;
  double  shininess;
  int     smoothsteps;
  colour  specularcolour;
  double  splinefactor;
  double  stickradius;
  double  sticktaper;
  double  strandthickness;
  double  strandwidth;
  double  transparency;
};

extern state *current_state;

void state_init (void);
void new_state (void);
void push_state (void);
void pop_state (void);

void set_atomcolour (void);
void set_atomcolour_bfactor (void);
void set_atomradius (void);
void set_bonddistance (void);
void set_bondcross (void);
void set_coilradius (void);
void set_colourparts (boolean on);
void set_colourramphsb (boolean hsb);
void set_cylinderradius (void);
void set_depthcue (void);
void set_emissivecolour (void);
void set_helixthickness (void);
void set_helixwidth (void);
void set_hsbrampreverse (boolean on);
void set_labelbackground (void);
void set_labelcentre (boolean on);
void set_labelclip (boolean on);
void set_labelmask (const char *str);
void set_labeloffset (void);
void set_labelrotation (boolean on);
void set_labelsize (void);
void set_lightambientintensity (void);
void set_lightattenuation (void);
void set_lightcolour (void);
void set_lightintensity (void);
void set_lightradius (void);
void set_linecolour (void);
void set_linedash (void);
void set_linewidth (void);
void set_objecttransform (boolean on);
void set_planecolour (void);
void set_plane2colour (void);
void set_residuecolour (void);
void set_residuecolour_bfactor (void);
void set_residuecolour_seq (void);
void set_regularexpression (boolean on);
void set_segments (void);
void set_segmentsize (void);
void set_shading (void);
void set_shadingexponent (void);
void set_shininess (void);
void set_smoothsteps (void);
void set_specularcolour (void);
void set_splinefactor (void);
void set_stickradius (void);
void set_sticktaper (void);
void set_strandthickness (void);
void set_strandwidth (void);
void set_transparency (void);

#endif
