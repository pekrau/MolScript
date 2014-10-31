%{
/* MolScript v2.1.2
 
   bison (GNU's yacc) parser definition

   Copyright (C) 1997-1998 Per Kraulis
     1-Dec-1996  first attempts
    17-Dec-1996  nearly complete syntax
     5-Jan-1997  fairly finished
     5-Feb-1997  added command_group for anchor and similar
    15-Sep-1997  basically finished
    24-Feb-1998  problem with token X under Linux: changed to XAXIS
    23-Jul-1998  rearranged syntax for anchor command
*/

#include "global.h"
#include "lex.h"
#include "col.h"
#include "coord.h"
#include "xform.h"
#include "state.h"
#include "graphics.h"
#include "select.h"
#include "postscript.h"
#include "raster3d.h"
#include "vrml.h"

/* To avoid warning with 'cc -ansi -fullwarn' on SGI IRIX; ugly! */
static void
__yy_bcopy (char *from, char *to, int count);

static void
__yy_memcpy (char *from, char *to, int count);


%}

%token INTEGER REAL STRING ITEM ON OFF POSITION TITLE MACRO PLOT END_PLOT
%token COMMENT DEBUG POSTSCRIPT RASTER3D VRML
%token NOFRAME FRAME AREA BACKGROUND WINDOW SLAB HEADLIGHT SHADOWS FOG
%token READ INLINE_PDB DELETE COPY ANCHOR DESCRIPTION PARAMETER VIEWPOINT
%token ORIGIN DIRECTIONALLIGHT POINTLIGHT SPOTLIGHT LEVEL_OF_DETAIL
%token TRANSFORM BY CENTRE TRANSLATION ROTATION XAXIS YAXIS ZAXIS AXIS
%token STORE_MATRIX RECALL_MATRIX
%token NOT REQUIRE AND EITHER OR BACKBONE PEPTIDE HYDROGENS ATOM RES_ATOM
%token B_FACTOR OCCUPANCY IN SPHERE CLOSE MODEL AMINO_ACIDS WATERS NUCLEOTIDES
%token LIGANDS MOLECULE FROM TO RESIDUE TYPE CONTAINS CHAIN ELEMENT SEGID
%token SET PUSH POP ATOMCOLOUR ATOMRADIUS BONDDISTANCE BONDCROSS COILRADIUS
%token COLOURPARTS COLOURRAMP CYLINDERRADIUS DEPTHCUE EMISSIVECOLOUR
%token HELIXTHICKNESS HELIXWIDTH HSBRAMPREVERSE LABELBACKGROUND LABELCENTRE
%token LABELCLIP LABELMASK LABELOFFSET LABELROTATION LABELSIZE
%token LIGHTAMBIENTINTENSITY LIGHTATTENUATION LIGHTCOLOUR LIGHTINTENSITY
%token LIGHTRADIUS LINECOLOUR LINEDASH LINEWIDTH OBJECTTRANSFORM PLANECOLOUR
%token PLANE2COLOUR REGULAREXPRESSION RESIDUECOLOUR SEGMENTS SEGMENTSIZE
%token SHADING SHADINGEXPONENT SHININESS SMOOTHSTEPS SPECULARCOLOUR
%token SPLINEFACTOR STICKRADIUS STICKTAPER STRANDTHICKNESS STRANDWIDTH
%token TRANSPARENCY
%token BALL_AND_STICK BONDS COIL CYLINDER CPK HELIX LABEL LINE OBJECT INLINE
%token STRAND TRACE TURN DOUBLE_HELIX
%token RGB HSB GREY RAINBOW

%%

file_contents : title plots;

title : TITLE id { set_title (yytext) }
      |
      ;

plots : plot
      | plot plots
      ;

plot : macro_defs PLOT { start_plot() }
         plot_contents END_PLOT { output_finish_plot() } ;

plot_contents : header_commands body_commands
              | body_commands
              ;

macro_defs : macro_def macro_defs
           |
           ;

macro_def : MACRO id { lex_define_macro (yytext) } ;

header_commands : header_command
                | header_command header_commands
                ;

header_command : NOFRAME               { frame = FALSE } /* no semi-colon! */
               | FRAME OFF ';'         { frame = FALSE }
               | FRAME ON ';'          { frame = TRUE }
               | AREA number number number number ';' { set_area() }
               | BACKGROUND colour ';' { set_background() }
               | WINDOW number ';'     { set_window() }
               | SLAB number ';'       { set_slab() }
               | HEADLIGHT ON ';'      { headlight = TRUE }
               | HEADLIGHT OFF ';'     { headlight = FALSE }
               | SHADOWS ON ';'        { shadows = TRUE }
               | SHADOWS OFF ';'       { shadows = FALSE }
               | FOG number ';'        { set_fog() }
               ;

body_commands : body_command
              | body_command body_commands
              ;

body_command : coord_command
             | geom_command
             | state_command
             | utility_command
             | ctrl_command
             ;

coord_command : READ id          { store_molname (yytext) } coordinates
              | DELETE id        { delete_molecule (yytext) } ';'
              | COPY id { lex_yytext_push() } atom_selection ';'
                  { lex_yytext_pop(); copy_molecule (yytext) }
              | TRANSFORM atom_selection { xform_init() }
                  xforms ';' { xform_atoms() }
              | STORE_MATRIX ';' { xform_store() }
              ;

coordinates : id { read_coordinate_file (yytext) } ';'
            | INLINE_PDB ';' { read_coordinate_file (NULL) }
            ;

xforms : BY xform
       | BY xform xforms
       ;

xform : CENTRE vector                 { xform_centre() }
      | TRANSLATION vector            { xform_translation() }
      | ROTATION XAXIS number         { xform_rotation_x() }
      | ROTATION YAXIS number         { xform_rotation_y() }
      | ROTATION ZAXIS number         { xform_rotation_z() }
      | ROTATION AXIS number number number number { xform_rotation_axis() }
      | ROTATION number number number
                 number number number
                 number number number { xform_rotation_matrix() }
      | RECALL_MATRIX                 { xform_recall_matrix() }
      ;

geom_command : BALL_AND_STICK atom_selection ';'   { ball_and_stick (TRUE) }
             | BALL_AND_STICK
                 atom_selection atom_selection ';' { ball_and_stick (FALSE) }
             | BONDS atom_selection ';'            { bonds (TRUE) }
             | BONDS atom_selection atom_selection ';' { bonds (FALSE) }
             | COIL residue_selection ';'          { coil (TRUE, TRUE) }
             | CYLINDER residue_selection ';'      { cylinder() }
             | CPK atom_selection ';'              { cpk() }
             | DOUBLE_HELIX residue_selection ';'  { coil (FALSE, FALSE) }
             | HELIX residue_selection ';'         { helix() }
             | LABEL vector id { label_position (yytext) } ';'
             | LABEL atom_selection id { label_atoms (yytext) } ';'
             | LINE vector { line_start() } lines ';' { output_line (TRUE) }
             | OBJECT object
             | STRAND residue_selection ';'        { strand() }
             | TRACE residue_selection ';'         { trace() }
             | TURN residue_selection ';'          { coil (TRUE, FALSE) }
             ;

lines : TO vector { line_next() }
      | TO vector { line_next() } lines
      ;

object : INLINE ';'                 { object (NULL) }
       | id { lex_yytext_push() } ';' { lex_yytext_pop(); object (yytext) }
       ;

state_command : SET   { new_state() } state_changes ';'
	      | PUSH  { push_state() } ';'
	      | POP   { pop_state() } ';'
              ;

state_changes : state_change
              | state_change ',' state_changes
              ;

state_change : ATOMCOLOUR atom_selection colour { set_atomcolour() }
             | ATOMCOLOUR atom_selection
                 B_FACTOR number number ramp    { set_atomcolour_bfactor() }
             | ATOMRADIUS atom_selection number { set_atomradius() }
             | BONDDISTANCE number              { set_bonddistance() }
             | BONDCROSS number                 { set_bondcross() }
             | COILRADIUS number                { set_coilradius() }
             | COLOURPARTS ON                   { set_colourparts (TRUE) }
             | COLOURPARTS OFF                  { set_colourparts (FALSE) }
             | COLOURRAMP HSB                   { set_colourramphsb (TRUE) }
             | COLOURRAMP RGB                   { set_colourramphsb (FALSE) }
             | CYLINDERRADIUS number            { set_cylinderradius() }
             | DEPTHCUE number                  { set_depthcue() }
             | EMISSIVECOLOUR colour            { set_emissivecolour() }
             | HELIXTHICKNESS number            { set_helixthickness() }
             | HELIXWIDTH number                { set_helixwidth() }
             | HSBRAMPREVERSE ON                { set_hsbrampreverse (TRUE) }
             | HSBRAMPREVERSE OFF               { set_hsbrampreverse (FALSE) }
             | LABELBACKGROUND number           { set_labelbackground() }
             | LABELCENTRE ON                   { set_labelcentre (TRUE) }
             | LABELCENTRE OFF                  { set_labelcentre (FALSE) }
             | LABELCLIP ON                     { set_labelclip (TRUE) }
             | LABELCLIP OFF                    { set_labelclip (FALSE) }
             | LABELMASK id                     { set_labelmask (yytext) }
             | LABELOFFSET vector               { set_labeloffset() }
             | LABELROTATION ON                 { set_labelrotation (TRUE) }
             | LABELROTATION OFF                { set_labelrotation (FALSE) }
             | LABELSIZE number                 { set_labelsize() }
             | LIGHTAMBIENTINTENSITY number     { set_lightambientintensity () }
             | LIGHTATTENUATION vector          { set_lightattenuation() }
             | LIGHTCOLOUR colour               { set_lightcolour() }
             | LIGHTINTENSITY number            { set_lightintensity () }
             | LIGHTRADIUS number               { set_lightradius () }
             | LINECOLOUR colour                { set_linecolour() }
             | LINEDASH number                  { set_linedash() }
             | LINEWIDTH number                 { set_linewidth() }
             | OBJECTTRANSFORM ON               { set_objecttransform (TRUE) }
             | OBJECTTRANSFORM OFF              { set_objecttransform (FALSE) }
             | PLANECOLOUR colour               { set_planecolour() }
             | PLANE2COLOUR colour              { set_plane2colour() }
             | REGULAREXPRESSION ON             { set_regularexpression (TRUE) }
             | REGULAREXPRESSION OFF            { set_regularexpression (FALSE) }
             | RESIDUECOLOUR residue_selection colour { set_residuecolour() }
             | RESIDUECOLOUR residue_selection
                 B_FACTOR number number ramp    { set_residuecolour_bfactor() }
             | RESIDUECOLOUR residue_selection ramp { set_residuecolour_seq() }
             | SEGMENTS INTEGER                 { set_segments() }
             | SEGMENTSIZE number               { set_segmentsize() }
             | SHADING number                   { set_shading() }
             | SHADINGEXPONENT number           { set_shadingexponent() }
             | SHININESS number                 { set_shininess() }
             | SMOOTHSTEPS INTEGER              { set_smoothsteps() }
             | SPECULARCOLOUR colour            { set_specularcolour() }
             | SPLINEFACTOR number              { set_splinefactor() }
             | STICKRADIUS number               { set_stickradius() }
             | STICKTAPER number                { set_sticktaper() }
             | STRANDTHICKNESS number           { set_strandthickness() }
             | STRANDWIDTH number               { set_strandwidth() }
             | TRANSPARENCY number              { set_transparency() }
             ;

utility_command : COMMENT id    { output_comment (yytext) } ';'
	        | DEBUG id      { debug (yytext) } ';'
                | macro_def ';'
                ;

ctrl_command : ANCHOR id { anchor_start (yytext) } anchor_description
                 anchor_parameters { anchor_start_geometry() }
                 '{' basic_commands '}' ';' { anchor_finish() }
             | LEVEL_OF_DETAIL { lod_start() } lod_blocks { lod_start_group() }
                 lod_group { lod_finish_group() } ';' { lod_finish() }
             | VIEWPOINT id { viewpoint_start (yytext) } view_definition ';'
             | DIRECTIONALLIGHT vector ';' { output_directionallight() }
             | DIRECTIONALLIGHT direction ';' { output_directionallight() }
             | POINTLIGHT vector ';' { output_pointlight() }
             | SPOTLIGHT vector vector number ';' { output_spotlight() }
             | SPOTLIGHT vector direction number ';' { output_spotlight() }
             ;

anchor_description : DESCRIPTION id { anchor_description (yytext) }
                   |
                   ;

anchor_parameters : anchor_parameter anchor_parameters
                  |
                  ;

anchor_parameter : PARAMETER id { anchor_parameter (yytext) } ;

basic_commands : basic_command
               | basic_command basic_commands
               ;

basic_command : geom_command
              | state_command
              | utility_command
              ;

lod_blocks : lod_block
           | lod_block lod_blocks
           ;

lod_block : number { lod_start_group() } lod_group { lod_finish_group() } ;

lod_group : '{' '}'
          | '{' basic_commands '}'
          ;

view_definition : direction { viewpoint_output() }
                | direction number { viewpoint_output() }
                | ORIGIN vector number { viewpoint_output() }
                ;

atom_selection : NOT atom_selection { select_atom_not() }
               | REQUIRE atom_selection atom_and
               | EITHER atom_selection atom_or
               | atom_specification
               ;

atom_and : AND atom_selection { select_atom_and() }
         | ',' atom_selection { select_atom_and() } atom_and
         ;

atom_or : OR atom_selection { select_atom_or() }
        | ',' atom_selection { select_atom_or() } atom_or
        ;

atom_specification : ATOM id                     { select_atom_id (yytext) }
                   | RES_ATOM id { lex_yytext_push() }
                              id { select_atom_res_id (yytext) }
                   | OCCUPANCY number number     { select_atom_occupancy() }
                   | B_FACTOR number number      { select_atom_b_factor() }
                   | IN residue_selection        { select_atom_in() }
                   | SPHERE vector number        { select_atom_sphere() }
                   | CLOSE atom_selection number { select_atom_close() }
                   | PEPTIDE                     { select_atom_peptide() }
                   | BACKBONE                    { select_atom_backbone() }
                   | HYDROGENS                   { select_atom_hydrogens() }
                   | ELEMENT id                  { select_atom_element (yytext) }
                   ;

residue_selection : NOT residue_selection { select_residue_not() }
                  | REQUIRE residue_selection residue_and
                  | EITHER residue_selection residue_or
                  | residue_specification
                  ;

residue_and : AND residue_selection { select_residue_and() }
            | ',' residue_selection { select_residue_and() } residue_and
            ;

residue_or : OR residue_selection { select_residue_or() }
           | ',' residue_selection { select_residue_or() } residue_or
           ;

residue_specification : MOLECULE id       { select_residue_molecule (yytext) }
                      | MODEL INTEGER     { select_residue_model() }
                      | FROM number_as_id { lex_yytext_push() }
                          TO number_as_id { select_residue_from_to
					      (lex_yytext_str(), yytext);
			                    lex_yytext_pop() }
                      | RESIDUE id        { select_residue_id (yytext) }
                      | TYPE id           { select_residue_type (yytext) }
                      | CHAIN id          { select_residue_chain (yytext) }
                      | CONTAINS atom_selection { select_residue_contains() }
                      | AMINO_ACIDS       { select_residue_amino_acids() }
                      | WATERS            { select_residue_waters() }
                      | NUCLEOTIDES       { select_residue_nucleotides() }
                      | LIGANDS           { select_residue_ligands() }
                      | SEGID id          { select_residue_segid (yytext) }
                      ;

vector : POSITION atom_selection { position() }
       | number number number
       ;

direction : FROM vector TO vector ;

colour : RGB number number number { set_rgb() }
       | HSB number number number { set_hsb() }
       | GREY number              { set_grey() }
       | id                       { set_colour (yytext) }
       ;

ramp : FROM colour { ramp_from_colour = given_colour }
         TO colour { set_colour_ramp (&given_colour) }
     | RAINBOW     { set_rainbow_ramp() }
     ;

number_as_id : number { pop_dstack (1) }
             | id
             ;
             
number : INTEGER
       | REAL
       ;

id : STRING
   | ITEM
   ;

%%


/*------------------------------------------------------------*/
int
yyerror (char *s)
{
  if (s) {
    fprintf (stderr, "Error: %s\n", s);
  } else {
    fprintf (stderr, "Error\n");
  }
  lex_info();
  if (exit_on_error) exit (1);
  lex_cleanup();
  clear_dstack();
  return 0;
}


/*------------------------------------------------------------*/
void
yywarning (char *s)
{
  if (s) {
    fprintf (stderr, "Warning: %s\n", s);
  } else {
    fprintf (stderr, "Warning\n");
  }
  lex_info();
}


/*------------------------------------------------------------*/
int
main (int argc, char *argv[])
{
  global_init();
  lex_init();
  process_arguments (&argc, argv);
  banner();
  yyparse();
  output_finish_output();
  banner();
  return 0;
}
