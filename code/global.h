/* global.h

   MolScript v2.1.2

   Global stuff.

   Copyright (C) 1997-1998 Per Kraulis
     1-Dec-1996  first attempts
    12-Sep-1997  rearranged modes
*/

#ifndef GLOBAL_H
#define GLOBAL_H 1

#include <stdio.h>

#include "clib/boolean.h"
#include "clib/vector3.h"
#include "clib/colour.h"

#define UNDEFINED_MODE  0
#define POSTSCRIPT_MODE 1
#define RASTER3D_MODE   2
#define VRML_MODE       3
#define OPENGL_MODE     4
#define EPS_MODE        5
#define JPEG_MODE       6
#define SGI_MODE        7
#define PNG_MODE        8
#define GIF_MODE        9

#define PRINT(str) fprintf(outfile,"%s",(str))

extern const char program_str[];
extern const char copyright_str[];
extern char user_str[];

extern int output_mode;

extern char *input_filename;
extern char *output_filename;
extern char *tmp_filename;
extern FILE *outfile;
extern boolean message_mode;
extern boolean exit_on_error;
extern boolean pretty_format;
extern int output_width;
extern int output_height;

#define MAX_DSTACK 10
extern double dstack [MAX_DSTACK];
extern int dstack_size;
extern int ival;

extern vector3 xaxis;
extern vector3 yaxis;
extern vector3 zaxis;

extern char *title;
extern int first_plot;

int yyerror (char *s);
int yyparse (void);
void yywarning (char *s);

void global_init(void);

void push_double (double f);
void clear_dstack (void);
void pop_dstack (int slots);

void do_nothing (void);
void do_nothing_str (char *str);

void banner (void);
void not_implemented (const char *str);

void process_arguments (int *argcp, char *argv[]);
void set_outfile (const char *mode);

void set_title (const char *str);
void start_plot (void);

void debug (const char *str);

#endif
