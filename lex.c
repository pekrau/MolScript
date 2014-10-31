/* lex.c

   MolScript v2.1.2

   Input lexer.

   Copyright (C) 1997-1998 Per Kraulis
    14-Dec-1996  first attempts
     3-Jan-1997  fairly finished
    20-Jan-1997  added curly braces for VRML anchor and similar
     4-Sep-1997  use double-hash table for macro storage
    21-Dec-1997  fixed memory leak in igetc
    12-Jan-1998  fixed reading of real values starting with decimal point
    15-Jan-1998  added yytext stack
    23-Jan-1998  better error trace; no crash if empty input file
    24-Feb-1998  fixed bug in lex_cleanup
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "clib/str_utils.h"
#include "clib/double_hash.h"

#include "lex.h"
#include "global.h"
#include "molscript.tab.h"

#define YYTEXT_SIZE 2048


/*------------------------------------------------------------*/
char yytext [YYTEXT_SIZE + 1];
int yylen;

typedef struct s_yytext_entry yytext_entry;

struct s_yytext_entry {
  char *yytext;
  int yylen;
  yytext_entry *prev;
};

yytext_entry *yytext_top = NULL;
  

/*------------------------------------------------------------*/
typedef struct s_input_source input_source;

struct s_input_source {
  char *name;
  FILE *file;
  char *macro;
  int line_number;
  input_source *prev;
};

typedef struct {
  char *name;
  char *contents;
} macro;

static int opened_files = 0;
static dhash_table *macro_table = NULL;
static int defining_macro = FALSE;
static int test_numerical = TRUE;


/*------------------------------------------------------------*/
static input_source *in_source = NULL;

/*------------------------------------------------------------*/
#define KEYWORD_SIZE 64

typedef struct {
  char word [KEYWORD_SIZE + 1];
  int code;
  int test_numerical;
} keyword;

static size_t total_keywords;
static keyword keywords[] =
{
  {"amino-acids", AMINO_ACIDS, TRUE},
  {"anchor", ANCHOR, TRUE},
  {"and", AND, TRUE},
  {"area", AREA, TRUE},
  {"atom", ATOM, FALSE},
  {"atomcolour", ATOMCOLOUR, TRUE},
  {"atomradius", ATOMRADIUS, TRUE},
  {"axis", AXIS, TRUE},
  {"b-factor", B_FACTOR, TRUE},
  {"backbone", BACKBONE, TRUE},
  {"background", BACKGROUND, TRUE},
  {"ball-and-stick", BALL_AND_STICK, TRUE},
  {"bondcross", BONDCROSS, TRUE},
  {"bonddistance", BONDDISTANCE, TRUE},
  {"bonds", BONDS, TRUE},
  {"by", BY, TRUE},
  {"centre", CENTRE, TRUE},
  {"chain", CHAIN, FALSE},
  {"close", CLOSE, TRUE},
  {"coil", COIL, TRUE},
  {"coilradius", COILRADIUS, TRUE},
  {"colourparts", COLOURPARTS, TRUE},
  {"colourramp", COLOURRAMP, TRUE},
  {"comment", COMMENT, TRUE},
  {"contains", CONTAINS, TRUE},
  {"copy", COPY, FALSE},
  {"cpk", CPK, TRUE},
  {"cylinder", CYLINDER, TRUE},
  {"cylinderradius", CYLINDERRADIUS, TRUE},
  {"debug", DEBUG, TRUE},
  {"delete", DELETE, FALSE},
  {"depthcue", DEPTHCUE, TRUE},
  {"description", DESCRIPTION, TRUE},
  {"directionallight", DIRECTIONALLIGHT, TRUE},
  {"double-helix", DOUBLE_HELIX, TRUE},
  {"either", EITHER, TRUE},
  {"element", ELEMENT, TRUE},
  {"emissivecolour", EMISSIVECOLOUR, TRUE},
  {"end_plot", END_PLOT, TRUE},
  {"fog", FOG, TRUE},
  {"frame", FRAME, TRUE},
  {"from", FROM, TRUE},
  {"gray", GREY, TRUE},
  {"grey", GREY, TRUE},
  {"headlight", HEADLIGHT, TRUE},
  {"helix", HELIX, TRUE},
  {"helixthickness", HELIXTHICKNESS, TRUE},
  {"helixwidth", HELIXWIDTH, TRUE},
  {"hsb", HSB, TRUE},
  {"hsbrampreverse", HSBRAMPREVERSE, TRUE},
  {"hydrogens", HYDROGENS, TRUE},
  {"in", IN, TRUE},
  {"inline", INLINE, FALSE},
  {"inline-PDB", INLINE_PDB, FALSE},
  {"label", LABEL, TRUE},
  {"labelbackground", LABELBACKGROUND, TRUE},
  {"labelcentre", LABELCENTRE, TRUE},
  {"labelclip", LABELCLIP,TRUE},
  {"labelmask", LABELMASK, TRUE},
  {"labeloffset", LABELOFFSET, TRUE},
  {"labelrotation", LABELROTATION, TRUE},
  {"labelsize", LABELSIZE, TRUE},
  {"level-of-detail", LEVEL_OF_DETAIL, TRUE},
  {"ligands", LIGANDS, TRUE},
  {"lightambientintensity", LIGHTAMBIENTINTENSITY, TRUE},
  {"lightattenuation", LIGHTATTENUATION, TRUE},
  {"lightcolour", LIGHTCOLOUR, TRUE},
  {"lightintensity", LIGHTINTENSITY, TRUE},
  {"lightradius", LIGHTRADIUS, TRUE},
  {"line", LINE, TRUE},
  {"linecolour", LINECOLOUR, TRUE},
  {"linedash", LINEDASH, TRUE},
  {"linewidth", LINEWIDTH, TRUE},
  {"macro", MACRO, TRUE},
  {"model", MODEL, TRUE},
  {"molecule", MOLECULE, FALSE},
  {"noframe", NOFRAME, TRUE},
  {"not", NOT, TRUE},
  {"nucleotides", NUCLEOTIDES, TRUE},
  {"object", OBJECT, TRUE},
  {"objecttransform", OBJECTTRANSFORM, TRUE},
  {"occupancy", OCCUPANCY, TRUE},
  {"off", OFF, TRUE},
  {"on", ON, TRUE},
  {"or", OR, TRUE},
  {"origin", ORIGIN, TRUE},
  {"parameter", PARAMETER, TRUE},
  {"peptide", PEPTIDE, TRUE},
  {"plane2colour", PLANE2COLOUR, TRUE},
  {"planecolour", PLANECOLOUR, TRUE},
  {"plot", PLOT, TRUE},
  {"pointlight", POINTLIGHT, TRUE},
  {"pop", POP, TRUE},
  {"position", POSITION, TRUE},
  {"postscript", POSTSCRIPT, FALSE},
  {"push", PUSH, TRUE},
  {"rainbow", RAINBOW, TRUE},
  {"raster3d", RASTER3D, FALSE},
  {"read", READ, FALSE},
  {"recall-matrix", RECALL_MATRIX, TRUE},
  {"regularexpression", REGULAREXPRESSION, TRUE},
  {"require", REQUIRE, TRUE},
  {"res-atom", RES_ATOM, FALSE},
  {"residue", RESIDUE, FALSE},
  {"residuecolour", RESIDUECOLOUR, TRUE},
  {"rgb", RGB, TRUE},
  {"rotation", ROTATION, TRUE},
  {"segid", SEGID, TRUE},
  {"segments", SEGMENTS, TRUE},
  {"segmentsize", SEGMENTSIZE, TRUE},
  {"set", SET, TRUE},
  {"shading", SHADING, TRUE},
  {"shadingexponent", SHADINGEXPONENT, TRUE},
  {"shadows", SHADOWS, TRUE},
  {"shininess", SHININESS, TRUE},
  {"slab", SLAB, TRUE},
  {"smoothsteps", SMOOTHSTEPS, TRUE},
  {"specularcolour", SPECULARCOLOUR, TRUE},
  {"sphere", SPHERE, TRUE},
  {"splinefactor", SPLINEFACTOR, TRUE},
  {"spotlight", SPOTLIGHT, TRUE},
  {"stickradius", STICKRADIUS, TRUE},
  {"sticktaper", STICKTAPER, TRUE},
  {"store-matrix", STORE_MATRIX, TRUE},
  {"strand", STRAND, TRUE},
  {"strandthickness", STRANDTHICKNESS, TRUE},
  {"strandwidth", STRANDWIDTH, TRUE},
  {"string", STRING, TRUE},
  {"title", TITLE, TRUE},
  {"to", TO, TRUE},
  {"trace", TRACE, TRUE},
  {"transform", TRANSFORM, TRUE},
  {"translation", TRANSLATION, TRUE},
  {"transparency", TRANSPARENCY, TRUE},
  {"turn", TURN, TRUE},
  {"type", TYPE, FALSE},
  {"viewpoint", VIEWPOINT, TRUE},
  {"vrml", VRML, FALSE},
  {"waters", WATERS, TRUE},
  {"window", WINDOW, TRUE},
  {"x", XAXIS, TRUE},
  {"y", YAXIS, TRUE},
  {"z", ZAXIS, TRUE},
  {"", 0, FALSE}		/* sentinel for end-of-array */
};


/*------------------------------------------------------------*/
void
lex_init (void)
{
  keyword *kw = keywords;

  total_keywords = 0;
  while (kw->code != 0) {
#ifndef NDEBUG
    if (total_keywords) assert (strcmp (kw->word, (kw-1)->word) > 0);
#endif
    total_keywords++;
    kw++;
  }

  if (macro_table) {
    int slot;
    macro *mac;

    for (slot = 0; slot < macro_table->size; slot++) {
      mac = (macro *) macro_table->objects[slot];
      if (mac) {
	free (mac->name);
	free (mac->contents);
	free (mac);
      }
    }

    dhash_delete (macro_table);
    macro_table = NULL;
  }

  in_source = malloc (sizeof (input_source));
  in_source->file = stdin;
  in_source->name = NULL;
  in_source->line_number = 1;
  in_source->prev = NULL;
}


/*------------------------------------------------------------*/
static void
lex_trace (input_source *is, char *word)
{
  assert (is);

  fprintf (stderr, " : line number %i", is->line_number);
  if (is->name) {
    if (is->file) {
      fprintf (stderr, " in file %s", is->name);
    } else {
      fprintf (stderr, " in macro %s", is->name);
    }
  }
  if (word) fprintf (stderr, ", at \"%s\"", word);
  fprintf (stderr, "\n");
}


/*------------------------------------------------------------*/
void
lex_info (void)
{
  if (in_source == NULL) {
    fprintf (stderr, " : no input\n");
  } else {
    input_source *is;
    lex_trace (in_source, yytext);
    for (is = in_source->prev; is; is = is->prev) lex_trace (is, NULL);
  }
}


/*------------------------------------------------------------*/
void
lex_cleanup (void)
{
  while (in_source && in_source->prev) {
    input_source *prev = in_source->prev;
    if (in_source->file) {
      if (in_source->file != stdin) {
	fclose (in_source->file);
	opened_files--;
	free (in_source->name);
      }
    } else {
      free (in_source->name);
    }
    free (in_source);
    in_source = prev;
  }
}


/*------------------------------------------------------------*/
void
lex_set_input_file (const char *filename)
{
  assert (filename);
  assert (*filename);

  in_source->file = fopen (filename, "r");
  opened_files++;
  in_source->name = str_clone (filename);
}


/*------------------------------------------------------------*/
FILE *
lex_input_file (void)
{
  return in_source->file;
}


/*------------------------------------------------------------*/
static int
igetc (void)
{
  int c;

  if (in_source->file) {
    c = fgetc (in_source->file);
    if (c == EOF) {
      input_source *prev = in_source->prev;
      if (defining_macro) {
	yyerror ("end-of-file reached while defining macro");
	defining_macro = FALSE;
      }
      if (in_source->file != stdin) {
	fclose (in_source->file);
	opened_files--;
	if (message_mode && (prev != NULL))
	  fprintf (stderr, "end of input stream file %s\n", in_source->name);
      }
      free (in_source->name);
      free (in_source);
      in_source = prev;
      if (in_source) c = igetc();
    }

  } else {
    c = *(in_source->macro++);
    if (c == '\0') {
      input_source *prev = in_source->prev;
      free (in_source->name);
      free (in_source);
      in_source = prev;
      c = igetc();
    }
  }

  return c;
}


/*------------------------------------------------------------*/
static void
iungetc (int c)
{
  if (in_source->file) {
    ungetc (c, in_source->file);
  } else {
    in_source->macro--;
  }
}


/*------------------------------------------------------------*/
static void
add_to_yytext (int c)
{
  if (yylen > YYTEXT_SIZE) {
    yytext[yylen--] = '\0';
    exit_on_error = TRUE;
    yyerror ("lexical item too long; parameter YYTEXT_SIZE in file lex.c");
  }
  yytext[yylen++] = c;
}


/*------------------------------------------------------------*/
static void
get_yytext (int c)
{
  for (;;) {
    switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
    case ',':
    case ';':
    case '}':
    case '{':
      iungetc (c);
    case EOF:
      yytext[yylen] = '\0';
      return;
    default:
      add_to_yytext (c);
    }
    c = igetc();
  }
}


/*------------------------------------------------------------*/
static void
push_in_source (int is_macro)
{
  input_source *is;
  macro *mac;

  get_yytext (igetc());

  is = malloc (sizeof (input_source));

  if (is_macro) {
    if (yylen == 0) {
      exit_on_error = TRUE;
      yyerror ("no macro name given for '$'");
    }

    mac = (macro *) dhash_object (macro_table, yytext);
    if (mac == NULL) {
      exit_on_error = TRUE;
      yyerror ("no such macro defined");
    }

    is->file = NULL;
    is->macro = mac->contents;

    if (message_mode)
      fprintf (stderr, "input stream macro %s\n", mac->name);
    
  } else {
    if (yylen == 0) {
      exit_on_error = TRUE;
      yyerror ("no file name given for '@'");
    }
    if (opened_files >= FOPEN_MAX) {
      exit_on_error = TRUE;
      yyerror ("could not open a new file; operating system maximum reached");
    }

    is->file = fopen (yytext, "r");
    is->macro = NULL;
    if (is->file == NULL) {
      exit_on_error = TRUE;
      yyerror ("could not open the given input stream file");
    }
    opened_files++;

    if (message_mode)
      fprintf (stderr, "input stream file %s\n", yytext);
  }

  is->name = str_clone (yytext);
  is->line_number = 1;
  is->prev = in_source;
  in_source = is;

  yylen = 0;
}


/*------------------------------------------------------------*/
static int
keyword_compare (const void *key, const void *base)
{
  return (strcmp ((char *) key, ((keyword *) base)->word));
}


/*------------------------------------------------------------*/
static void
skip_comment (void)
{
  int c = igetc();
  for (;;) {
    switch (c) {
    case '\n':
      in_source->line_number++;
      return;
    case EOF:
      return;
    default:
      c = igetc();
    }
  }
}


/*------------------------------------------------------------*/
static int
get_string (void)
{
  int c;

  for (c = igetc(); c != '"'; c = igetc()) {
    switch (c) {
    case '\n':
    case EOF:
      goto unfinished;
    case '\\':
      c = igetc();
      switch (c) {
      case '\n':
      case EOF:
	goto unfinished;
      case '"':
	break;
      default:
	add_to_yytext (c);
      }
    }
    add_to_yytext (c);
  }

  if (yylen == 0) {
    exit_on_error = TRUE;
    yyerror ("empty quoted string");
  }

  yytext[yylen] = '\0';
  return STRING;

unfinished:
  exit_on_error = TRUE;
  yyerror ("unfinished quoted string");
}


/*------------------------------------------------------------*/
static int
get_item (int c)
{
  keyword *kw;

  get_yytext (c);

  kw = (keyword *) bsearch (yytext, keywords, total_keywords,
			    sizeof (keyword), keyword_compare);

  if (kw) {
    test_numerical = kw->test_numerical;
    return kw->code;

  } else if (test_numerical) {
    char *p = yytext;
    if ((*p == '+') || (*p == '-')) p++; /* optional sign */
    if (*p != '.') {		         /* decimal point first; real */
      if (! isdigit (*p)) return ITEM;   /* first char must be digit */
      p++;
      while (isdigit (*p)) p++;	         /* following digits */
      if (*p == '\0') {		         /* at end; is an integer */
	ival = atoi (yytext);
	push_double (atof (yytext));
	return INTEGER;
      }
      if (*p != '.') return ITEM;        /* non-digit must be decimal point */
    }
    p++;
    while (isdigit (*p)) p++;	         /* digits following decimal point */
    if (*p == '\0') {		         /* at end; is a real */
      push_double (atof (yytext));
      return REAL;
    }
    if (! ((*p == 'e') || (*p == 'E'))) return ITEM; /* optional exponent */
    p++;
    if ((*p == '+') || (*p == '-')) p++; /* optional sign */
    while (isdigit (*p)) p++;	         /* following digits */
    if (*p == '\0') {		         /* at end; is a real */
      push_double (atof (yytext));
      return REAL;
    }

  } else {
    test_numerical = TRUE;
  }

  return ITEM;
}


/*------------------------------------------------------------*/
void
lex_define_macro (char *name)
{
  macro *mac;
  int size = 64;
  char item [10];
  int item_length, pos;
  char c;

  assert (name);
  assert (*name);

  defining_macro = TRUE;

  fflush (NULL);
  if (macro_table == NULL) {
    macro_table = dhash_create (32);
  } else if (dhash_slot (macro_table, name) >= 0) {
    exit_on_error = TRUE;
    yyerror ("macro already defined");
  }

  mac = malloc (sizeof (macro));
  mac->name = str_clone (name);
  mac->contents = malloc (size * sizeof (char));

  pos = dhash_insert (macro_table, mac->name, mac);
  assert (pos);

  pos = 0;
  item_length = 0;

  for (;;) {
    c = igetc();
    switch (c) {

    case '!':
      skip_comment();
      break;

    case '\n':
      in_source->line_number++;

    default:
      item [item_length++] = c;
      if (str_eqn (item, "end_macro", item_length)) {
	if (item_length == 9) {
	  mac->contents [pos] = '\0';
	  goto skip;
	}
      } else {
	int slot;
	if (pos >= size - item_length) {
	  size *= 2;
	  mac->contents = realloc (mac->contents, size * sizeof (char));
	}
	for (slot = 0; slot < item_length; slot++) {
	  mac->contents [pos++] = item [slot];
	}
	item_length = 0;
      }
    }
  }

 skip:
  defining_macro = FALSE;
}


/*------------------------------------------------------------*/
void
lex_yytext_push (void)
{
  yytext_entry *new;
#ifndef NDEBUG
  int old_depth = lex_yytext_depth();
#endif

  assert (yylen);

  new = malloc (sizeof (yytext_entry));
  new->yytext = str_clone (yytext);
  new->yylen = yylen;
  new->prev = yytext_top;
  yytext_top = new;

  assert (yytext_top);
  assert (str_eq (yytext_top->yytext, yytext));
#ifndef NDEBUG
  assert (lex_yytext_depth() == old_depth + 1);
#endif
}


/*------------------------------------------------------------*/
void
lex_yytext_pop (void)
{
  yytext_entry *prev;
#ifndef NDEBUG
  int old_depth = lex_yytext_depth();
#endif

  assert (yytext_top);

  strcpy (yytext, yytext_top->yytext);
  yylen = yytext_top->yylen;

  prev = yytext_top->prev;
  free (yytext_top->yytext);
  free (yytext_top);

  yytext_top = prev;

#ifndef NDEBUG
  assert (lex_yytext_depth() == old_depth - 1);
#endif
}


/*------------------------------------------------------------*/
int
lex_yytext_depth (void)
{
  yytext_entry *curr;
  int count = 0;

  for (curr = yytext_top; curr; curr = curr->prev) count++;
  return count;
}


/*------------------------------------------------------------*/
char *
lex_yytext_str (void)
{
  assert (yytext_top);

  return yytext_top->yytext;
}


/*------------------------------------------------------------*/
int
yylex (void)
{
  int c;

  yylen = 0;
  for (c = igetc(); c != EOF; c = igetc()) {
    switch (c) {
    case ' ':
    case '\t':
    case '\v':
    case '\f':
    case '\r':
      break;
    case '\n':
      in_source->line_number++;
      break;
    case '!':
      skip_comment();
      break;
    case ',':
    case ';':
    case '{':
    case '}':
      yytext[0] = c;
      yytext[1] = '\0';
      yylen = 1;
      return c;
    case '"':
      return (get_string());
    case '@':
      push_in_source (FALSE);
      break;
    case '$':
      push_in_source (TRUE);
      break;
    default:
      return (get_item (c));
    }
  }

  return 0;
}
