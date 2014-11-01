/* mol3d_utils

   Molecule coordinate data file utility procedures.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
     4-May-1998  broken out of mol3d
*/

#include "mol3d_utils.h"

/* public ====================
#include <stdio.h>

#include <mol3d.h>
#include <mol3d_init.h>
==================== public */

#include <assert.h>
#include <stdlib.h>

#include <str_utils.h>


/*------------------------------------------------------------*/
void
mol3d_fprintf_count (FILE *file, mol3d *mol)
     /*
       Print a report on the number of residues and atoms in the molecule.
     */
{
  /* pre */
  assert (file);
  assert (mol);

  fprintf (file,
	   "Molecule coordinate set %s: %i residues, %i atoms.\n",
	   mol->name ? mol->name : "-",
	   mol3d_count_residues (mol),
	   mol3d_count_atoms (mol));
}


/*------------------------------------------------------------*/
void
mol3d_fprintf_count_all (FILE *file, mol3d *first_mol)
     /*
       Print a report on the number of molecules, residues and atoms
       in the linked list from the molecule.
     */
{
  /* pre */
  assert (file);
  assert (first_mol);

  fprintf (file,
	   "Molecule coordinate set: %i molecules, %i residues, %i atoms.\n",
	   mol3d_count (first_mol),
	   mol3d_count_residues_all (first_mol),
	   mol3d_count_atoms_all (first_mol));
}


/*------------------------------------------------------------*/
void
mol3d_fprintf_residues (FILE *file, mol3d *mol)
     /*
       Print a list of all residues in the molecule.
     */
{
  res3d *res;
  at3d *at;

  /* pre */
  assert (file);
  assert (mol);

  for (res = mol->first; res; res = res->next) {
    fprintf (file, "residue %c %s %s (%s, %i, %c, %c, %c) %i\n",
	     res->chain, res->name, res->type, res->segid, res->ordinal,
	     res->code, res->secstruc, res->heterogen ? 'T' : 'F',
	     res3d_count_atoms (res));
  }
}


/*------------------------------------------------------------*/
at3d **
mol3d_atom_list (mol3d *mol)
     /*
       Return a list (NULL-ended array) of pointers to all atoms
       in the molecule.
     */
{
  int count = 0;
  at3d **list;
  res3d *res;
  at3d *at;

  /* pre */
  assert (mol);

  list = malloc ((mol3d_count_atoms (mol) + 1) * sizeof (at3d *));

  for (res = mol->first; res; res = res->next) {
    for (at = res->first; at; at = at->next) list[count++] = at;
  }
  list[count] = NULL;

  return list;
}


/*------------------------------------------------------------*/
at3d **
mol3d_atom_list_all (mol3d *first_mol)
     /*
       Return a list (NULL-ended array) of pointers to all atoms
       in the molecule and its linked list.
     */
{
  int count = 0;
  at3d **list;
  mol3d *mol;
  res3d *res;
  at3d *at;

  /* pre */
  assert (mol);

  list = malloc ((mol3d_count_atoms_all (mol) + 1) * sizeof (at3d *));

  for (mol = first_mol; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) list[count++] = at;
    }
  }
  list[count] = NULL;

  return list;
}


/*------------------------------------------------------------*/
res3d **
mol3d_residue_list (mol3d *mol)
     /*
       Return a list (NULL-ended array) of pointers to all residues
       in the molecule.
     */
{
  int count = 0;
  res3d **list;
  res3d *res;

  /* pre */
  assert (mol);

  list = malloc ((mol3d_count_residues (mol) + 1) * sizeof (res3d *));

  for (res = mol->first; res; res = res->next) list[count++] = res;
  list[count] = NULL;

  return list;
}


/*------------------------------------------------------------*/
res3d **
mol3d_residue_list_all (mol3d *first_mol)
     /*
       Return a list (NULL-ended array) of pointers to all residues
       in the molecule and its linked list.
     */
{
  int count = 0;
  res3d **list;
  mol3d *mol;
  res3d *res;

  /* pre */
  assert (mol);

  list = malloc ((mol3d_count_residues_all (mol) + 1) * sizeof (res3d *));

  for (mol = first_mol; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) list[count++] = res;
  }
  list[count] = NULL;

  return list;
}


/*------------------------------------------------------------*/
int
mol3d_delete_altloc_atoms (mol3d *mol)
     /*
       Delete all atoms in the molecule with alternate positions not
       designated 'A' in the PDB coordinate set.
       Return the number of deleted atoms.
     */
{
  res3d *res;
  at3d *at, *atnext;
  int count = 0;

  /* pre */
  assert (mol);

  for (res = mol->first; res; res = res->next) {
    at = res->first;
    while (at) {
      if (at->altloc == 'A') {
	at->altloc = ' ';
      } else if (at->altloc != ' ') {
	atnext = at->next;
	res3d_remove_atom (res, at);
	at3d_delete (at);
	at = atnext;
	count ++;
      } else {
	at = at->next;
      }
    }
  }

  return count;
}


/*------------------------------------------------------------*/
int
mol3d_delete_residue_type (mol3d *mol, char *type)
     /*
       Delete all residues of the given type in the molecule.
       Return the number of residues deleted.
     */
{
  res3d *curr, *next;
  int count = 0;

  /* pre */
  assert (mol);
  assert (type);
  assert (*type);

  curr = mol->first;
  for (curr = mol->first; curr; curr = next) {
    next = curr->next;
    if (str_eq (type, curr->type)) {
      if (curr->prev) {
	curr->prev->next = next;
      } else {
	mol->first = next;
      }
      if (next) next->prev = curr->prev;
      curr->prev = NULL;
      curr->next = NULL;
      res3d_delete (curr);
      count++;
    }
  }

  return count;
}


/*------------------------------------------------------------*/
void
mol3d_extent (mol3d *mol, vector3 *low, vector3 *high)
     /*
       Return the low and high 3D points forming a box containing
       all atoms (with their current radii) in the given molecule.
       The initial values in the input low and high vectors are significant.
     */
{
  register double c, p, r;
  double xlo, ylo, zlo, xhi, yhi, zhi;
  res3d *res;
  at3d *at;

  /* pre */
  assert (mol);
  assert (mol->init & MOL3D_INIT_RADII);
  assert (low);
  assert (high);

  xlo = low->x;
  ylo = low->y;
  zlo = low->z;
  xhi = high->x;
  yhi = high->y;
  zhi = high->z;

  for (res = mol->first; res; res = res->next) {
    for (at = res->first; at; at = at->next) {
      r = at->radius;
      p = at->xyz.x;
      c = p - r;
      if (c < xlo) xlo = c;
      c = p + r;
      if (c > xhi) xhi = c;
      p = at->xyz.y;
      c = p - r;
      if (c < ylo) ylo = c;
      c = p + r;
      if (c > yhi) yhi = c;
      p = at->xyz.z;
      c = p - r;
      if (c < zlo) zlo = c;
      c = p + r;
      if (c > zhi) zhi = c;
    }
  }

  low->x = xlo;
  low->y = ylo;
  low->z = zlo;
  high->x = xhi;
  high->y = yhi;
  high->z = zhi;
}


/*------------------------------------------------------------*/
double
mol3d_max_radius (mol3d *mol)
     /*
       Return the maximum atom radius in the molecule.
     */
{
  res3d *res;
  at3d *at;
  double radius = 0.0;

  /* pre */
  assert (mol);
  assert (mol->init & MOL3D_INIT_RADII);

  for (res = mol->first; res; res = res->next) {
    for (at = res->first; at; at = at->next) {
      if (radius < at->radius) radius = at->radius;
    }
  }

  return radius;
}


/*------------------------------------------------------------*/
boolean
res3d_unique_name (mol3d *mol, res3d *res)
     /*
       Is the name of the given residue unique within the given molecule?
     */
{
  res3d *res2;

  /* pre */
  assert (mol);
  assert (res);

  for (res2 = mol->first; res2; res2 = res2->next) {
    if (str_eq (res2->name, res->name)) {
      if (res2 != res) return FALSE;
    }
  }

  return TRUE;
}


/*------------------------------------------------------------*/
res3d *
res3d_find_aa_number (mol3d *mol, int number)
     /*
       Return the amino-acid residue of the given sequential number
       (starting at 0) in the given residue. Residues having a code other
       than 'X' are considered amino-acid residues. Return NULL if not found.
     */
{
  res3d *res;

  /* pre */
  assert (mol);
  assert (mol->init & MOL3D_INIT_AACODES);
  assert (number >= 0);

  for (res = mol->first; res; res = res->next) {
    if ((res->code != 'X') && (--number == -1)) return res;
  }

  return NULL;
}


/*------------------------------------------------------------*/
int
res3d_aa_number (mol3d *mol, res3d *res)
     /*
       Return the sequential number (starting a 0) of the given amino-acid
       residue in the molecule. Residues with a code other than 'X' are
       considered amino-acid residues. Return -1 if not found.
     */
{
  res3d *res2;
  int number = -1;

  /* pre */
  assert (mol);
  assert (mol->init & MOL3D_INIT_AACODES);
  assert (res);
  assert (res->code != 'X');

  for (res2 = mol->first; res2; res2 = res2->next) {
    if (res2->code != 'X') {
      number++;
      if (res2 == res) return number;
    }
  }

  return -1;
}
