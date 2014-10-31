/* select.c

   MolScript v2.1.2

   Atom and residue selection routines.

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
     2-Jan-1997  largely finished
*/

#include <assert.h>
#include <stdlib.h>

#include "other/regex.h"

#include "clib/str_utils.h"
#include "clib/aa_lookup.h"
#include "clib/element_lookup.h"

#include "select.h"
#include "global.h"
#include "state.h"
#include "lex.h"


/*
#define SELECT_DEBUG 1
*/


/*============================================================*/
selection *current_atom_sel = NULL;
selection *current_residue_sel = NULL;



/*------------------------------------------------------------*/
static regexp *
compile_regexp (const char *str)
{
  regexp *pat;
  char *pattern = NULL;

  if (current_state->regularexpression) {
    pattern = str_clone (str);

  } else {			/* translate into regular expression */
    int pattern_size = 128;
    char *s, *p;

  start_again:
    if (pattern != NULL) free (pattern);
    pattern_size *= 2;
    pattern = malloc ((pattern_size + 1) * sizeof (char));

    s = (char *) str;
    p = pattern;
    *p++ = '^';

    while (*s != '\0') {
      switch (*s) {

      case '*':			/* any number of any characters */
	if ((p - pattern + 1) >= (pattern_size - 1)) goto start_again;
	*p++ = '.';
	*p++ = '*';
	break;

      case '%':			/* any single character */
	if (p - pattern >= (pattern_size - 1)) goto start_again;
	*p++ = '.';
	break;

      case '#':			/* any number of any digits */
	if ((p - pattern + 5) >= (pattern_size - 1)) goto start_again;
	*p++ = '[';
	*p++ = '0';
	*p++ = '-';
	*p++ = '9';
	*p++ = ']';
	*p++ = '*';
	break;

      case '+':			/* any single digit */
	if ((p - pattern + 4) >= (pattern_size - 1)) goto start_again;
	*p++ = '[';
	*p++ = '0';
	*p++ = '-';
	*p++ = '9';
	*p++ = ']';
	break;

      case '^':			/* special meaning in regular expression */
      case '$':
      case '[':
      case ']':
      case '.':
	if (p - pattern + 1 >= (pattern_size - 1)) goto start_again;
	*p++ = '\\';
	*p++ = *s;
	break;

      default:			/* ordinary character */
	if (p - pattern >= (pattern_size - 1)) goto start_again;
	*p++ = *s;
	break;
      }
      s++;
    }

    *p++ = '$';
    *p = '\0';
  }

  pat = regcomp (pattern);
  free (pattern);
  if (pat == NULL) yyerror ("invalid regular expression");
  return pat;
}


/*------------------------------------------------------------*/
void
push_atom_selection (void)
{
  selection *sel;
#ifndef NDEBUG
  int old = count_atom_selections();
#endif

  if (total_atoms == 0) yyerror ("no coordinates loaded");

  sel = malloc (sizeof (selection));
  sel->flags = malloc (total_atoms * sizeof (int));
  sel->next = NULL;
  if (current_atom_sel) {
    current_atom_sel->next = sel;
    sel->prev = current_atom_sel;
  } else {
    sel->prev = NULL;
  }
  current_atom_sel = sel;

  assert (count_atom_selections() == old + 1);
}


/*------------------------------------------------------------*/
void
push_residue_selection (void)
{
  selection *sel;
#ifndef NDEBUG
  int old = count_residue_selections();
#endif

  if (total_residues == 0) yyerror ("no coordinates loaded");

  sel = malloc (sizeof (selection));
  sel->flags = malloc (total_residues * sizeof (int));
  sel->next = NULL;
  if (current_residue_sel) {
    current_residue_sel->next = sel;
    sel->prev = current_residue_sel;
  } else {
    sel->prev = NULL;
  }
  current_residue_sel = sel;

  assert (count_residue_selections() == old + 1);
}


/*------------------------------------------------------------*/
void
pop_atom_selection (void)
{
  selection *sel;
#ifndef NDEBUG
  int old = count_atom_selections();
  assert (old >= 1);
#endif

  sel = current_atom_sel;
  if (sel->prev) {
    sel->prev->next = NULL;
    current_atom_sel = sel->prev;
  } else {
    current_atom_sel = NULL;
  }
  free (sel->flags);
  free (sel);

  assert (count_atom_selections() == old - 1);
}


/*------------------------------------------------------------*/
void
pop_residue_selection (void)
{
  selection *sel;
#ifndef NDEBUG
  int old = count_residue_selections();
  assert (old >= 1);
#endif

  sel = current_residue_sel;
  if (sel->prev) {
    sel->prev->next = NULL;
    current_residue_sel = sel->prev;
  } else {
    current_residue_sel = NULL;
  }
  free (sel->flags);
  free (sel);

  assert (count_residue_selections() == old - 1);
}


/*------------------------------------------------------------*/
int
count_atom_selections (void)
{
  selection *sel;
  int sum = 0;

  for (sel = current_atom_sel; sel; sel = sel->prev) sum++;
  return sum;
}


/*------------------------------------------------------------*/
int
count_residue_selections (void)
{
  selection *sel;
  int sum = 0;

  for (sel = current_residue_sel; sel; sel = sel->prev) sum++;
  return sum;
}


/*------------------------------------------------------------*/
int
select_atom_count (void)
{
  int slot;
  int sum = 0;
  int *flags;

  assert (current_atom_sel != NULL);

  flags = current_atom_sel->flags;
  for (slot = 0; slot < total_atoms; slot++) if (*flags++) sum++;
  return sum;
}


/*------------------------------------------------------------*/
int
select_residue_count (void)
{
  int slot;
  int sum = 0;
  int *flags;

  assert (current_residue_sel != NULL);

  flags = current_residue_sel->flags;
  for (slot = 0; slot < total_residues; slot++) if (*flags++) sum++;
  return sum;
}


/*------------------------------------------------------------*/
at3d **
select_atom_list (int *atom_count)
{
  int *flags;
  at3d **atoms;
  int slot;
#ifndef NDEBUG
  int old = count_atom_selections();
  assert (old >= 1);
#endif

  assert (atom_count);

  *atom_count = select_atom_count();

  if (*atom_count == 0) {
    atoms = NULL;

  } else {
    mol3d *mol;
    res3d *res;
    at3d *at;

    atoms = malloc (*atom_count * sizeof (at3d *));
    slot = 0;
    flags = current_atom_sel->flags;

    for (mol = first_molecule; mol; mol = mol->next) {
      for (res = mol->first; res; res = res->next) {
	for (at = res->first; at; at = at->next) {
	  if (*flags++) atoms [slot++] = at;
	}
      }
    }
  }

  pop_atom_selection();

#ifndef NDEBUG
  assert (count_atom_selections() == old - 1);
#endif

  return atoms;
}


/*------------------------------------------------------------*/
void
select_atom_not (void)
{
  int slot;
  int *flags;
#ifndef NDEBUG
  int old = count_atom_selections();
  assert (old >= 1);
#endif

  flags = current_atom_sel->flags;
  for (slot = 0; slot < total_atoms; slot++, flags++) *flags = !(*flags);

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select 'not' expression: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_and (void)
{
  int slot;
  int *flags1, *flags2;
#ifndef NDEBUG
  int old = count_atom_selections();
  assert (old >= 2);
#endif

  flags1 = current_atom_sel->prev->flags;
  flags2 = current_atom_sel->flags;
  for (slot = 0; slot < total_atoms; slot++, flags1++, flags2++) {
    *flags1 = (*flags1 && *flags2);
  }
  pop_atom_selection();

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select 'and' expression: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old - 1);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_or (void)
{
  int slot;
  int *flags1, *flags2;
#ifndef NDEBUG
  int old = count_atom_selections();
  assert (old >= 2);
#endif

  flags1 = current_atom_sel->prev->flags;
  flags2 = current_atom_sel->flags;
  for (slot = 0; slot < total_atoms; slot++, flags1++, flags2++) {
    *flags1 = (*flags1 || *flags2);
  }
  pop_atom_selection();

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select 'or' expression: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old - 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_not (void)
{
  int slot;
  int *flags;
#ifndef NDEBUG
  int old = count_residue_selections();
  assert (old >= 1);
#endif

  flags = current_residue_sel->flags;
  for (slot = 0; slot < total_residues; slot++, flags++) *flags = !(*flags);

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select 'not' expression: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_and (void)
{
  int slot;
  int *flags1, *flags2;
#ifndef NDEBUG
  int old = count_residue_selections();
  assert (old >= 2);
#endif

  flags1 = current_residue_sel->prev->flags;
  flags2 = current_residue_sel->flags;
  for (slot = 0; slot < total_residues; slot++, flags1++, flags2++) {
    *flags1 = (*flags1 && *flags2);
  }
  pop_residue_selection();

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select 'and' expression: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old - 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_or (void)
{
  int slot;
  int *flags1, *flags2;
#ifndef NDEBUG
  int old = count_residue_selections();
  assert (old >= 2);
#endif

  flags1 = current_residue_sel->prev->flags;
  flags2 = current_residue_sel->flags;
  for (slot = 0; slot < total_residues; slot++, flags1++, flags2++) {
    *flags1 = (*flags1 || *flags2);
  }
  pop_residue_selection();

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select 'or' expression: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old - 1);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_id (const char *item)
{
  mol3d *mol;
  res3d *res;
  at3d *at;
  regexp *rx;
  int *flags;
#ifndef NDEBUG
  int old = count_atom_selections();
#endif

  assert (item);
  assert (*item);

  rx = compile_regexp (item);
  push_atom_selection();
  flags = current_atom_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) {
	*flags++ = (regexec (rx, at->name) != 0);
      }
    }
  }

  free (rx);

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select id: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_res_id (const char *item)
{
#ifndef NDEBUG
  int old = count_atom_selections();
#endif

  select_atom_id (item);
  select_residue_id (lex_yytext_str());
  select_atom_in();
  select_atom_and();

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select res_atom: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_occupancy (void)
{
  mol3d *mol;
  res3d *res;
  at3d *at;
  double lower, upper;
  int *flags;
#ifndef NDEBUG
  int old = count_atom_selections();
#endif

  assert (dstack_size == 2);

  lower = dstack[0];
  upper = dstack[1];
  clear_dstack();

  if (upper < lower) {
    yyerror ("invalid range of occupancy");
    return;
  }

  push_atom_selection();
  flags = current_atom_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) {
	*flags++ = ((lower <= at->occupancy) && (at->occupancy <= upper));
      }
    }
  }

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select occupancy: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_b_factor (void)
{
  mol3d *mol;
  res3d *res;
  at3d *at;
  double lower, upper;
  int *flags;
#ifndef NDEBUG
  int old = count_atom_selections();
#endif

  assert (dstack_size == 2);

  lower = dstack[0];
  upper = dstack[1];
  clear_dstack();

  if (upper < lower) {
    yyerror ("invalid b-factor range values");
    return;
  }

  push_atom_selection();
  flags = current_atom_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) {
	*flags++ = ((lower <= at->bfactor) && (at->bfactor <= upper));
      }
    }
  }

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select b-factor: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_in (void)
{
  mol3d *mol;
  res3d *res;
  int *resflags, *atflags;
  at3d *at;
#ifndef NDEBUG
  int oldat = count_atom_selections();
  int oldres = count_residue_selections();
  assert (oldres >= 1);
#endif

  push_atom_selection();
  atflags = current_atom_sel->flags;
  resflags = current_residue_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) *atflags++ = *resflags;
      resflags++;
    }
  }

  pop_residue_selection();

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select in: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == oldat + 1);
  assert (count_residue_selections() == oldres - 1);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_sphere (void)
{
  mol3d *mol;
  res3d *res;
  at3d *at;
  vector3 centre;
  double sqradius;
  int *flags;
#ifndef NDEBUG
  int old = count_atom_selections();
#endif

  assert (dstack_size == 4);

  centre.x = dstack[0];
  centre.y = dstack[1];
  centre.z = dstack[2];
  sqradius = dstack[3];
  clear_dstack();

  if (sqradius < 0.0) {
    yyerror ("invalid radius value");
    return;
  }

  sqradius *= sqradius;

  push_atom_selection();
  flags = current_atom_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) {
	*flags++ = v3_close (&(at->xyz), &centre, sqradius);
      }
    }
  }

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select sphere: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_close (void)
{
  double sqdistance;
  int slot, atom_count;
  int *flags;
#ifndef NDEBUG
  int old = count_atom_selections();
#endif

  assert (dstack_size == 1);

  sqdistance = dstack[0];
  clear_dstack();

  if (sqdistance < 0.0) {
    yyerror ("invalid distance value");
    return;
  }

  sqdistance *= sqdistance;

  atom_count = select_atom_count();

  if (atom_count == 0) {
    flags = current_atom_sel->flags;
    for (slot = 0; slot < total_atoms; slot++) *flags++ = FALSE;

  } else {
    mol3d *mol;
    res3d *res;
    at3d *at;
    int flag;
    at3d **atoms, **close_atoms;
    close_atoms = malloc (atom_count * sizeof (at3d *));

    atoms = close_atoms;
    flags = current_atom_sel->flags;
    for (mol = first_molecule; mol; mol = mol->next) {
      for (res = mol->first; res; res = res->next) {
	for (at = res->first; at; at = at->next) {
	  if (*flags++) *atoms++ = at;
	}
      }
    }

    select_atom_not();
    push_atom_selection();

    flags = current_atom_sel->flags;
    for (mol = first_molecule; mol; mol = mol->next) {
      for (res = mol->first; res; res = res->next) {
	for (at = res->first; at; at = at->next) {
	  flag = FALSE;
	  atoms = close_atoms;
	  for (slot = 0; slot < atom_count; slot++, atoms++) {
	    if (v3_close (&(at->xyz), &((*atoms)->xyz), sqdistance)) {
	      flag = TRUE;
	      break;
	    }
	  }
	  *flags++ = flag;
	}
      }
    }

    select_atom_and();

    free (close_atoms);
  }

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select close: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_backbone (void)
{
  int old_regularexpression = current_state->regularexpression;
#ifndef NDEBUG
  int old = count_atom_selections();
#endif

  current_state->regularexpression = TRUE;
  select_atom_id ("^N$");
  select_atom_id ("^CA$");
  select_atom_or();
  select_atom_id ("^C$");
  select_atom_or();
  select_atom_id ("^O$");
  select_atom_or();
  select_residue_amino_acids();
  select_atom_in();
  select_atom_and();

  current_state->regularexpression = FALSE;
  select_atom_id ("*'");
  select_atom_id ("O%P");
  select_atom_or();
  select_atom_id ("P");
  select_atom_or();
  select_residue_amino_acids();
  select_residue_not();
  select_atom_in();
  select_atom_and();

  select_atom_or();

  current_state->regularexpression = old_regularexpression;

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select backbone: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_peptide (void)
{
  int old_regularexpression = current_state->regularexpression;
#ifndef NDEBUG
  int old = count_atom_selections();
#endif

  current_state->regularexpression = TRUE;
  select_atom_id ("^N$");
  select_atom_id ("^C$");
  select_atom_or();
  select_atom_id ("^O$");
  select_atom_or();
  select_residue_amino_acids();
  select_atom_in();
  select_atom_and();

  current_state->regularexpression = old_regularexpression;

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select peptide: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_hydrogens (void)
{
  int old_regularexpression = current_state->regularexpression;
#ifndef NDEBUG
  int old = count_atom_selections();
#endif

  current_state->regularexpression = TRUE;

  select_atom_id ("^H.*");
  select_atom_id ("^1H.*");
  select_atom_or();
  select_atom_id ("^2H.*");
  select_atom_or();
  select_atom_id ("^3H.*");
  select_atom_or();

  current_state->regularexpression = old_regularexpression;

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select hydrogen: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_atom_element (const char *item)
{
  mol3d *mol;
  res3d *res;
  at3d *at;
  int element;
  int *flags;
#ifndef NDEBUG
  int old = count_atom_selections();
#endif

  assert (item);
  assert (*item);

  element = element_number_convert (item);
  push_atom_selection();
  flags = current_atom_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) {
	*flags++ = (at->element == element);
      }
    }
  }

#ifdef SELECT_DEBUG
  fprintf (stderr, "atom select element: %i\n", select_atom_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_molecule (const char *item)
{
  int *flags;
  regexp *rx;
  mol3d *mol;
  res3d *res;
  int flag;
#ifndef NDEBUG
  int old = count_residue_selections();
#endif

  assert (item);
  assert (*item);

  rx = compile_regexp (item);
  push_residue_selection();
  flags = current_residue_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    flag = (regexec (rx, mol->name) != 0);
    for (res = mol->first; res; res = res->next) *flags++ = flag;
  }

  free (rx);

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select molecule: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_model (void)
{
  mol3d *mol;
  res3d *res;
  int *flags;
  int flag, model;
#ifndef NDEBUG
  int old = count_residue_selections();
#endif

  assert (dstack_size == 1);

  model = ival;
  clear_dstack();

  push_residue_selection();
  flags = current_residue_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    flag = mol->model == model;
    for (res = mol->first; res; res = res->next) *flags++ = flag;
  }

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select model: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_from_to (const char *item1, const char *item2)
{
  mol3d *mol;
  res3d *res;
  boolean within_sequence = FALSE;
  boolean first_is_known = FALSE;
  boolean first_was_aa;
  regexp *rx1, *rx2;
  int *flags;
#ifndef NDEBUG
  int old = count_residue_selections();
#endif

  assert (item1);
  assert (*item1);
  assert (item2);
  assert (*item2);

  if (str_eq (item1, item2)) {
    yyerror ("invalid from-to selection: same residue");
    return;
  }

  rx1 = compile_regexp (item1);
  rx2 = compile_regexp (item2);
  push_residue_selection();
  flags = current_residue_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      if (within_sequence) {
	*flags++ = TRUE;
	if (regexec (rx2, res->name) != 0) within_sequence = FALSE;
      } else {
	within_sequence = regexec (rx1, res->name) != 0;
	if (within_sequence) {	/* kludge to deal with the silly PDB */
	  if (first_is_known) {	/* notion of allowing multiple residues */
	    if (first_was_aa) {	/* with the same name in a file */
	      if (res->code == 'X') within_sequence = FALSE;
	    }
	  } else {
	    first_was_aa = res->code != 'X';
	    first_is_known = TRUE;
	  }
	}
	*flags++ = within_sequence;
      }
    }
  }

  if (within_sequence) yywarning ("sequence segment not ended (residue selection \"from to\")");

  free (rx1);
  free (rx2);

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select from to: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_id (const char *item)
{
  mol3d *mol;
  res3d *res;
  regexp *rx;
  int *flags;
#ifndef NDEBUG
  int old = count_residue_selections();
#endif

  assert (item);
  assert (*item);

  rx = compile_regexp (item);
  push_residue_selection();
  flags = current_residue_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      *flags++ = regexec (rx, res->name);
    }
  }

  free (rx);

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select id: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_type (const char *item)
{
  mol3d *mol;
  res3d *res;
  regexp *rx;
  int *flags;
#ifndef NDEBUG
  int old = count_residue_selections();
#endif

  assert (item);
  assert (*item);

  rx = compile_regexp (item);
  push_residue_selection();
  flags = current_residue_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      *flags++ = regexec (rx, res->type);
    }
  }

  free (rx);

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select type: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_chain (const char *item)
{
  mol3d *mol;
  res3d *res;
  char chain;
  int *flags;
#ifndef NDEBUG
  int old = count_residue_selections();
#endif

  assert (item);
  assert (*item);

  chain = *item;
  push_residue_selection();
  flags = current_residue_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      *flags++ = res->chain == chain;
    }
  }

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select chain: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_contains (void)
{
  mol3d *mol;
  res3d *res;
  at3d *at;
  int *resflags, *atflags;
#ifndef NDEBUG
  int oldat = count_atom_selections();
  int oldres = count_residue_selections();
  assert (oldat >= 1);
#endif

  atflags = current_atom_sel->flags;
  push_residue_selection();
  resflags = current_residue_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      *resflags = FALSE;
      for (at = res->first; at; at = at->next) {
	if (*atflags++) *resflags = TRUE;
      }
      resflags++;
    }
  }

  pop_atom_selection();

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select contains: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_atom_selections() == oldat - 1);
  assert (count_residue_selections() == oldres + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_amino_acids (void)
{
  mol3d *mol;
  res3d *res;
  int *flags;
#ifndef NDEBUG
  int old = count_residue_selections();
#endif

  push_residue_selection();
  flags = current_residue_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      *flags++ = res->code != 'X';
    }
  }

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select amino-acids: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_waters (void)
{
  mol3d *mol;
  res3d *res;
  int *flags;
#ifndef NDEBUG
  int old = count_residue_selections();
#endif

  push_residue_selection();
  flags = current_residue_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      *flags++ = is_water_type (res->type);
    }
  }

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select waters: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_nucleotides (void)
{
  mol3d *mol;
  res3d *res;
  int *flags;
#ifndef NDEBUG
  int old = count_residue_selections();
#endif

  push_residue_selection();
  flags = current_residue_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      *flags++ = is_nucleic_acid_type (res->type);
    }
  }

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select nucleotides: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_ligands (void)
{
#ifndef NDEBUG
  int old = count_residue_selections();
#endif

  select_residue_amino_acids();
  select_residue_waters();
  select_residue_or();
  select_residue_nucleotides();
  select_residue_or();
  select_residue_not();

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select ligands: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old + 1);
#endif
}


/*------------------------------------------------------------*/
void
select_residue_segid (const char *item)
{
  mol3d *mol;
  res3d *res;
  int *flags;
#ifndef NDEBUG
  int old = count_residue_selections();
#endif

  assert (item);
  assert (*item);

  push_residue_selection();
  flags = current_residue_sel->flags;

  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      *flags++ = str_eq (res->segid, item);
    }
  }

#ifdef SELECT_DEBUG
  fprintf (stderr, "residue select segid: %i\n", select_residue_count());
#endif
#ifndef NDEBUG
  assert (count_residue_selections() == old + 1);
#endif
}
