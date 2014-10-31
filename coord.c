/* coord.c

   MolScript v2.1.2

   Coordinate data handling.

   Copyright (C) 1997-1998 Per Kraulis
     3-Dec-1996  first attempts
     3-Jan-1997  fairly finished
    30-Jan-1997  use clib mol3d
*/

#include <assert.h>
#include <stdlib.h>

#include "clib/str_utils.h"

#include "coord.h"
#include "global.h"
#include "lex.h"
#include "select.h"


/*============================================================*/
mol3d *first_molecule = NULL;
int total_atoms = 0;
int total_residues = 0;

static char *molname = NULL;


/*------------------------------------------------------------*/
void
store_molname (char *name)
{
  assert (name);
  assert (*name);

  if (molname) free (molname);
  molname = str_clone (name);
}


/*------------------------------------------------------------*/
void
read_coordinate_file (char *filename)
{
  mol3d *mol = NULL;
  mol3d *mol2;
  res3d *res;
  at3d *at;
  int mol_count = 0;
  int res_count= 0;
  int at_count = 0;

  if (filename) {

    if (mol3d_is_pdb_code (filename)) {
      if (message_mode) fprintf (stderr, "reading PDB data set...\n");
      mol = mol3d_read_pdb_code (filename);

    } else {
      switch (mol3d_file_type (filename)) {
      case MOL3D_UNKNOWN_FILE:
      case MOL3D_PDB_FILE:
	if (message_mode) fprintf (stderr, "reading PDB file...\n");
	mol = mol3d_read_pdb_filename (filename);
	break;
      case MOL3D_MSA_FILE:
	not_implemented ("MSA coordinate file format");
	break;
      case MOL3D_DG_FILE:
	not_implemented ("DG coordinate file format");
	break;
      case MOL3D_RD_FILE:
	not_implemented ("RD coordinate file format");
	break;
      case MOL3D_CDS_FILE:
	not_implemented ("CDS coordinate file format");
	break;
      case MOL3D_WAH_FILE:
	not_implemented ("WAH coordinate file format");
	break;
      }
    }

  } else {
    char ch;
    FILE *file = lex_input_file();
    if (message_mode)
      fprintf (stderr, "reading inline PDB coordinate data...\n");
    while ((ch = fgetc (file)) != '\n') {
      if (ch == EOF) yyerror ("no inline PDB coordinate data");
    }
    mol = mol3d_read_pdb_file (file);
  }

  if (mol == NULL) {
    yyerror ("no molecule read; could not open file, or file format error");
    return;
  }

  for (mol2 = mol; mol2; mol2 = mol2->next) {
    mol_count++;

    mol3d_set_name (mol2, molname);
    mol3d_init (mol2,
		MOL3D_INIT_NOBLANKS | MOL3D_INIT_COLOURS | MOL3D_INIT_RADII |
		MOL3D_INIT_AACODES | MOL3D_INIT_CENTRALS |
		MOL3D_INIT_ATOM_ORDINALS | MOL3D_INIT_ELEMENTS);
    mol3d_init_residue_ordinals_protein (mol2);

    for (res = mol2->first; res; res = res->next) {/* change '*' to ''' to */
      res_count++;		                   /* avoid clash with */
      for (at = res->first; at; at = at->next) {   /* MolScript wildcard */
	at_count++;
	str_exchange (at->name, '*', '\'');
      }
    }
  }

  if (message_mode) {
    if (mol_count > 1) fprintf (stderr, "%i models, ", mol_count);
    fprintf (stderr, "%i residues and %i atoms read into molecule %s\n",
	     res_count, at_count, mol->name);
  }

  if (first_molecule) {
    mol3d_append (first_molecule, mol);
  } else {
    first_molecule = mol;
  }

  update_totals();
}


/*------------------------------------------------------------*/
void
update_totals (void)
{
  if (first_molecule) {
    total_residues = mol3d_count_residues_all (first_molecule);
    total_atoms = mol3d_count_atoms_all (first_molecule);
  } else {
    total_residues = 0;
    total_atoms = 0;
  }
}


/*------------------------------------------------------------*/
void
copy_molecule (char *name)
{
  mol3d *mol, *new_mol;
  res3d *res, *first_res, *new_res;
  res3d *prev_res = NULL;
  res3d *curr_res = NULL;
  at3d *curr_at = NULL;
  at3d *at, *new_at;
  int *flags;
  int rescount = 0;
  int atcount= 0;

  assert (name);
  assert (*name);
  assert (count_atom_selections() == 1);

  new_mol = mol3d_create();
  mol3d_set_name (new_mol, name);

  flags = current_atom_sel->flags;
  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) {
	if (*flags++) {

	  new_at = at3d_clone (at);
	  atcount++;

	  if (prev_res != res) {
	    prev_res = res;

	    new_res = res3d_clone (res);
	    rescount++;

	    if (curr_res) {
	      res3d_add (curr_res, new_res);
	    } else {
	      first_res = new_res;
	      mol3d_append_residue (new_mol, first_res);
	    }
	    curr_res = new_res;
	    res3d_append_atom (curr_res, new_at);

	  } else {
	    at3d_add (curr_at, new_at);
	  }

	  curr_at = new_at;
	}
      }
    }
  }
  pop_atom_selection();

  mol3d_init (new_mol,
	      MOL3D_INIT_NOBLANKS | MOL3D_INIT_COLOURS | MOL3D_INIT_RADII |
	      MOL3D_INIT_AACODES | MOL3D_INIT_CENTRALS |
	      MOL3D_INIT_ATOM_ORDINALS | MOL3D_INIT_ELEMENTS);
  mol3d_init_residue_ordinals_protein (new_mol);

  mol3d_append (first_molecule, new_mol);
  update_totals();

  if (message_mode)
    fprintf (stderr, "%i residues and %i atoms copied to molecule %s\n",
	     rescount, atcount, new_mol->name);

  assert (count_atom_selections() == 0);
}


/*------------------------------------------------------------*/
void
delete_molecule (char *name)
{
  mol3d *mol;
  boolean nothing_deleted = TRUE;

  assert (name);
  assert (*name);

  mol = first_molecule;
  while (mol) {
    if (str_eq (mol->name, name)) {
      mol3d *next = mol->next;
      named_data *nd;

      if (mol == first_molecule) {
	first_molecule = mol->next;
	mol->next = NULL;
      } else {
	mol3d_remove_molecule (first_molecule, mol);
      }

      if (message_mode) {
	fprintf (stderr, "deleting molecule %s", name);
	if (mol->model != 0) fprintf (stderr, " (model %i)", mol->model);
	fprintf (stderr, "\n");
      }

      mol3d_delete (mol);
      nothing_deleted = FALSE;

      mol = next;
    } else {
      mol = mol->next;
    }
  }

  update_totals();

  if (nothing_deleted) yyerror ("no such molecule to delete");
}


/*------------------------------------------------------------*/
void
delete_all_molecules (void)
{
  if (first_molecule) {
    mol3d_delete_all (first_molecule);
    first_molecule = NULL;
  }

  update_totals();
}


/*------------------------------------------------------------*/
mol3d_chain *
get_peptide_chains (void)
{
  mol3d_chain *ch;
#ifndef NDEBUG
  int old = count_residue_selections();
  assert (old >= 1);
#endif

  ch = mol3d_chain_find (first_molecule, PEPTIDE_CHAIN_ATOMNAME,
			 PEPTIDE_DISTANCE, current_residue_sel->flags);
  pop_residue_selection();

#ifndef NDEBUG
  assert (count_residue_selections() == old - 1);
#endif

  return ch;
}


/*------------------------------------------------------------*/
mol3d_chain *
get_nucleotide_chains (void)
{
  mol3d_chain *ch;
#ifndef NDEBUG
  int old = count_residue_selections();
  assert (old >= 1);
#endif

  ch = mol3d_chain_find (first_molecule, NUCLEOTIDE_CHAIN_ATOMNAME,
			 NUCLEOTIDE_DISTANCE, current_residue_sel->flags);
  pop_residue_selection();

#ifndef NDEBUG
  assert (count_residue_selections() == old - 1);
#endif

  return ch;
}


/*------------------------------------------------------------*/
void
position (void)
{
  mol3d *mol;
  res3d *res;
  at3d *at;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  int count = 0;
  int *flags;
#ifndef NDEBUG
  int old = count_atom_selections();
  assert (old >= 1);
#endif

  flags = current_atom_sel->flags;
  for (mol = first_molecule; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) {
	if (*flags++) {
	  x += at->xyz.x;
	  y += at->xyz.y;
	  z += at->xyz.z;
	  count++;
	}
      }
    }
  }
  pop_atom_selection();

  if (count > 0) {
    push_double (x / ((double) count));
    push_double (y / ((double) count));
    push_double (z / ((double) count));
    if (message_mode)
      fprintf (stderr, "%i atoms selected for position\n", count);
  } else {
    yyerror ("0 atoms selected for position");
    push_double (0.0);
    push_double (0.0);
    push_double (0.0);
  }

#ifndef NDEBUG
  assert (count_atom_selections() == old - 1);
#endif
}
