/* mol3d

   Molecule coordinate data structures and basic routines.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
     3-Dec-1996  first attempts
    30-Jan-1997  fairly finished
     2-Mar-1998  element number added in at3d
     5-May-1998  modified for hgen; segid introduced
    27-May-1998  added 'unique' unsigned integer parameters
    28-Dec-1998  removed bug in mol3d_delete
*/

#include "mol3d.h"

/* public ====================
#include <boolean.h>
#include <vector3.h>
#include <colour.h>
#include <named_data.h>

#define RES3D_NAME_LENGTH  6
#define RES3D_TYPE_LENGTH  4
#define RES3D_SEGID_LENGTH 4
#define AT3D_NAME_LENGTH   4

typedef struct s_mol3d mol3d;
typedef struct s_res3d res3d;
typedef struct s_at3d at3d;

struct s_mol3d {
  unsigned int unique;
  char *name;
  int model;
  res3d *first;
  mol3d *next;
  unsigned int init;
  named_data *data;
};

struct s_res3d {
  unsigned int unique;
  char name [RES3D_NAME_LENGTH + 1];
  char type [RES3D_TYPE_LENGTH + 1];
  char segid [RES3D_SEGID_LENGTH + 1];
  int ordinal;
  char chain, code, secstruc;
  boolean heterogen;
  double accessibility;
  colour colour;
  at3d *central;
  res3d *beta1, *beta2;
  res3d *prev, *next;
  at3d *first;
  mol3d *mol;
  named_data *data;
};

struct s_at3d {
  unsigned int unique;
  char name [AT3D_NAME_LENGTH + 1];
  int element;
  int ordinal;
  char altloc;
  vector3 xyz;
  double occupancy, bfactor, radius, charge, accessibility, rval;
  colour colour;
  at3d *next;
  res3d *res;
  named_data *data;
};

#define MOL3D_INIT_NOBLANKS         0x00000001
#define MOL3D_INIT_COLOURS          0x00000002
#define MOL3D_INIT_RADII            0x00000004
#define MOL3D_INIT_AACODES          0x00000008
#define MOL3D_INIT_CENTRALS         0x00000010
#define MOL3D_INIT_RESIDUE_ORDINALS 0x00000020
#define MOL3D_INIT_ATOM_ORDINALS    0x00000040
#define MOL3D_INIT_ELEMENTS         0x00000080
#define MOL3D_INIT_SECSTRUC         0x00000100
#define MOL3D_INIT_ACCESS           0x00000200
==================== public */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <str_utils.h>


/*============================================================*/
static unsigned int mol3d_unique = 0;
static unsigned int res3d_unique = 0;
static unsigned int at3d_unique = 0;


/*------------------------------------------------------------*/
mol3d *
mol3d_create (void)
{
  mol3d *mol = malloc (sizeof (mol3d));

  mol->unique = mol3d_unique++;
  mol->name = NULL;
  mol->model = 0;
  mol->first = NULL;
  mol->next = NULL;
  mol->init = 0;
  mol->data = NULL;

  return mol;
}


/*------------------------------------------------------------*/
res3d *
res3d_create (void)
{
  res3d *res = malloc (sizeof (res3d));

  res->unique = res3d_unique++;
  res->name[0] = '\0';
  res->type[0] = '\0';
  res->segid[0] = '\0';
  res->ordinal = 0;
  res->chain = ' ';
  res->code = '-';
  res->secstruc = '-';
  res->heterogen = FALSE;
  res->accessibility = 0.0;
  colour_set_grey (&(res->colour), 1.0);
  res->central = NULL;
  res->beta1 = NULL;
  res->beta2 = NULL;
  res->prev = NULL;
  res->next = NULL;
  res->first = NULL;
  res->mol = NULL;
  res->data = NULL;

  return res;
}


/*------------------------------------------------------------*/
at3d *
at3d_create (void)
{
  at3d *at = malloc (sizeof (at3d));

  at->unique = at3d_unique++;
  at->name[0] = '\0';
  at->element = 0;
  at->ordinal = 0;
  at->altloc = ' ';
  at->xyz.x = 0.0;
  at->xyz.y = 0.0;
  at->xyz.z = 0.0;
  at->occupancy = 0.0;
  at->bfactor = 0.0;
  at->radius = 1.7;
  at->charge = 0.0;
  at->accessibility = 0.0;
  colour_set_grey (&(at->colour), 1.0);
  at->next = NULL;
  at->res = NULL;
  at->data = NULL;

  return at;
}


/*------------------------------------------------------------*/
res3d *
res3d_clone (res3d *res)
     /*
       Clone the given residue; all values except pointers are copied.
       The atoms list is not copied.
      */
{
  res3d *new = malloc (sizeof (res3d));

  /* pre */
  assert (res);

  memcpy (new, res, sizeof (res3d));
  new->unique = res3d_unique++;
  new->central = NULL;
  new->beta1 = NULL;
  new->beta2 = NULL;
  new->prev = NULL;
  new->next = NULL;
  new->first = NULL;
  new->mol = NULL;
  new->data = NULL;

  return new;
}


/*------------------------------------------------------------*/
at3d *
at3d_clone (at3d *at)
     /*
       Clone the given atom; all values except pointers are copied.
      */
{
  at3d *new = malloc (sizeof (at3d));

  /* pre */
  assert (at);

  memcpy (new, at, sizeof (at3d));
  new->unique = at3d_unique++;
  new->next = NULL;
  new->res = NULL;
  new->data = NULL;

  return new;
}


/*------------------------------------------------------------*/
void
mol3d_delete (mol3d *mol)
     /*
       Delete the given molecule and all its residues and atoms.
      */
{
  res3d *res, *next;

  /* pre */
  assert (mol);

  if (mol->name) free (mol->name);
  for (res = mol->first; res; res = next) {
    next = res->next;
    res3d_delete (res);
  }
  if (mol->data) nd_delete (mol->data);
  free (mol);
}


/*------------------------------------------------------------*/
void
mol3d_delete_all (mol3d *first_mol)
     /*
       Delete the given molecule and all its residues and atoms,
       and recursively the molecules in the linked list.
      */
{
  /* pre */
  assert (first_mol);

  if (first_mol->next) mol3d_delete_all (first_mol->next);
  mol3d_delete (first_mol);
}


/*------------------------------------------------------------*/
void
res3d_delete (res3d *res)
     /*
       Delete the given residue, its atoms and data.
      */
{
  at3d *at, *next;

  /* pre */
  assert (res);

  for (at = res->first; at; at = next) {
    next = at->next;
    at3d_delete (at);
  }
  if (res->data) nd_delete (res->data);
  free (res);
}

/*------------------------------------------------------------*/
void
at3d_delete (at3d *at)
     /*
       Delete the atom and its data.
      */
{
  /* pre */
  assert (at);

  if (at->data) nd_delete (at->data);
  free (at);
}


/*------------------------------------------------------------*/
void
mol3d_set_name (mol3d *mol, const char *name)
     /*
       Set the molecule name; this is a separate copy.
      */
{
  /* pre */
  assert (mol);

  if (mol->name) free (mol->name);
  if (name && *name) {
    mol->name = str_clone (name);
  } else {
    mol->name = NULL;
  }
}


/*------------------------------------------------------------*/
mol3d *
mol3d_append (mol3d *first_mol, mol3d *new)
     /*
       Append the new molecule to the end of the linked list for the
       given molecule. Return the new molecule.
      */
{
  /* pre */
  assert (first_mol);
  assert (new);
  assert (new->next == NULL);

  while (first_mol->next) first_mol = first_mol->next;
  first_mol->next = new;

  return new;
}


/*------------------------------------------------------------*/
res3d *
res3d_add (res3d *res, res3d *new)
     /*
       Add the new residue into the linked list after the given residue.
       Return the new residue.
      */
{
  /* pre */
  assert (res);
  assert (new);
  assert (new->mol == NULL);
  assert (new->prev == NULL);
  assert (new->next == NULL);

  new->next = res->next;
  if (new->next) new->next->prev = new;
  res->next = new;
  new->prev = res;
  new->mol = res->mol;

  return new;
}


/*------------------------------------------------------------*/
at3d *
at3d_add (at3d *at, at3d *new)
     /*
       Add the new atom into the linked list after the given atom.
       Return the new atom.
      */
{
  /* pre */
  assert (at);
  assert (new);
  assert (new->res == NULL);
  assert (new->next == NULL);

  while (at->next) at = at->next;
  at->next = new;
  new->res = at->res;

  return new;
}


/*------------------------------------------------------------*/
res3d *
mol3d_append_residue (mol3d *mol, res3d *res)
     /*
       Append the residue to the molecule. Return the residue.
      */
{
  /* pre */
  assert (mol);
  assert (res);
  assert (res->mol == NULL);
  assert (res->prev == NULL);
  assert (res->next == NULL);

  if (mol->first) {
    res3d *prev = mol->first;
    while (prev->next) prev = prev->next;
    res3d_add (prev, res);
  } else {
    mol->first = res;
  }
  res->mol = mol;

  return res;
}


/*------------------------------------------------------------*/
at3d *
res3d_append_atom (res3d *res, at3d *at)
     /*
       Append the atom to the residue. Return the atom.
      */
{
  /* pre */
  assert (res);
  assert (at);
  assert (at->res == NULL);
  assert (at->next == NULL);

  if (res->first) {
    at3d *prev = res->first;
    while (prev->next) prev = prev->next;
    prev->next = at;
  } else {
    res->first = at;
  }
  at->res = res;

  return at;
}


/*------------------------------------------------------------*/
named_data *
mol3d_create_named_data (mol3d *mol, char *name, boolean ncopy,
			 void *data, boolean dcopy)
     /*
       Create a named data entry and add to the molecule.
       Return the named data entry.
      */
{
  named_data *new;

  /* pre */
  assert (mol);
  assert (name);
  assert (data);

  new = nd_create (name, ncopy);
  nd_set_data (new, data, dcopy);
  if (mol->data) {
    nd_append (mol->data, new);
  } else {
    mol->data = new;
  }

  return new;
}


/*------------------------------------------------------------*/
named_data *
res3d_create_named_data (res3d *res, char *name, boolean ncopy,
			 void *data, boolean dcopy)
     /*
       Create a named data entry and add to the residue.
       Return the named data entry.
      */
{
  named_data *new;

  /* pre */
  assert (res);
  assert (name);
  assert (data);

  new = nd_create (name, ncopy);
  nd_set_data (new, data, dcopy);
  if (res->data) {
    nd_append (res->data, new);
  } else {
    res->data = new;
  }

  return new;
}


/*------------------------------------------------------------*/
named_data *
at3d_create_named_data (at3d *at, char *name, boolean ncopy,
			void *data, boolean dcopy)
     /*
       Create a named data entry and add to the atom.
       Return the named data entry.
      */
{
  named_data *new;

  /* pre */
  assert (at);
  assert (name);
  assert (data);

  new = nd_create (name, ncopy);
  nd_set_data (new, data, dcopy);
  if (at->data) {
    nd_append (at->data, new);
  } else {
    at->data = new;
  }

  return new;
}


/*------------------------------------------------------------*/
boolean
mol3d_remove_molecule (mol3d *mol1, mol3d *mol2)
     /*
       Remove the second molecule from the linked list of the first.
       Return TRUE if it was in the list, FALSE otherwise.
      */
{
  mol3d *curr, *prev;

  /* pre */
  assert (mol1);
  assert (mol2);

  for (curr = mol1, prev = NULL;
       curr;
       prev = curr, curr = curr->next) {
    if (curr == mol2) goto found;
  }

  return FALSE;

found:
  if (prev) prev->next = curr->next;
  curr->next = NULL;
  return TRUE;
}


/*------------------------------------------------------------*/
boolean
mol3d_remove_residue (mol3d *mol, res3d *res)
     /*
       Remove the residue from the molecule.
       Return TRUE if it was in the molecule, FALSE otherwise.
      */
{
  res3d *curr, *prev;

  /* pre */
  assert (mol);
  assert (res);

  for (curr = mol->first, prev = NULL;
       curr;
       prev = curr, curr = curr->next) {
    if (curr == res) {
      if (prev) {
	prev->next = curr->next;
      } else {
	mol->first = curr->next;
      }
      curr->next = NULL;
      return TRUE;
    }
  }

  return FALSE;
}


/*------------------------------------------------------------*/
boolean
res3d_remove_atom (res3d *res, at3d *at)
     /*
       Remove the atom from the residue.
       Return TRUE if it was in the residue, FALSE otherwise.
      */
{
  at3d *curr, *prev;

  /* pre */
  assert (res);
  assert (at);

  for (curr = res->first, prev = NULL;
       curr;
       prev = curr, curr = curr->next) {
    if (curr == at) {
      if (prev) {
	prev->next = curr->next;
      } else {
	res->first = curr->next;
      }
      curr->next = NULL;
      return TRUE;
    }
  }

  return FALSE;
}


/*------------------------------------------------------------*/
int
mol3d_count (mol3d *first_mol)
     /*
       The number of molecules in the linked list from and including this.
      */
{
  int count = 0;
  mol3d *mol;

  /* pre */
  assert (first_mol);

  for (mol = first_mol; mol; mol = mol->next) count++;

  return count;
}


/*------------------------------------------------------------*/
int
mol3d_count_residues (mol3d *mol)
     /*
       The number of residues in the molecule.
      */
{
  int count = 0;
  res3d *res;

  /* pre */
  assert (mol);

  for (res = mol->first; res; res = res->next) count++;

  return count;
}


/*------------------------------------------------------------*/
int
mol3d_count_residues_all (mol3d *first_mol)
     /*
       The number of residues in the molecule and those in the linked list.
      */
{
  int count = 0;
  mol3d *mol;
  res3d *res;

  /* pre */
  assert (first_mol);

  for (mol = first_mol ; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) count++;
  }

  return count;
}


/*------------------------------------------------------------*/
int
mol3d_count_atoms (mol3d *mol)
     /*
       The number of atoms in the molecule.
      */
{
  int count = 0;
  res3d *res;
  at3d *at;

  /* pre */
  assert (mol);

  for (res = mol->first; res; res = res->next) {
    for (at = res->first; at; at = at->next) count++;
  }

  return count;
}


/*------------------------------------------------------------*/
int
mol3d_count_atoms_all (mol3d *first_mol)
     /*
       The number of atoms in the molecule and those in the linked list.
      */
{
  int count = 0;
  mol3d *mol;
  res3d *res;
  at3d *at;

  /* pre */
  assert (first_mol);

  for (mol = first_mol; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) count++;
    }
  }

  return count;
}


/*------------------------------------------------------------*/
int
res3d_count_atoms (res3d *res)
{
  int count = 0;
  at3d *at;

  /* pre */
  assert (res);

  for (at = res->first; at; at = at->next) count++;

  return count;
}


/*------------------------------------------------------------*/
mol3d *
mol3d_lookup (mol3d *first_mol, const char *name)
     /*
       Return the molecule in the linked list with the given name.
      */
{
  /* pre */
  assert (name);
  assert (*name);

  for (; first_mol; first_mol = first_mol->next) {
    if (first_mol->name && str_eq (first_mol->name, name)) break;
  }
  return first_mol;
}


/*------------------------------------------------------------*/
res3d *
res3d_lookup (mol3d *mol, const char *name)
     /*
       Return the residue in the given molecule with the given name.
      */
{
  res3d *res;

  /* pre */
  assert (mol);
  assert (name);
  assert (*name);

  for (res = mol->first; res; res = res->next) {
    if (str_eq (res->name, name)) break;
  }
  return res;
}


/*------------------------------------------------------------*/
at3d *
at3d_lookup (res3d *res, const char *name)
     /*
       Return the atom in the given residue with the given name.
      */
{
  at3d *at;

  /* pre */
  assert (res);
  assert (name);
  assert (*name);

  for (at = res->first; at; at = at->next) {
    if (str_eq (at->name, name)) break;
  }
  return at;
}


/*------------------------------------------------------------*/
res3d *
res3d_find_ordinal (mol3d *mol, int ordinal)
     /*
       Return the residue in the given molecule with the given ordinal.
      */
{
  res3d *res;

  /* pre */
  assert (mol);
  assert (ordinal >= 0);

  for (res = mol->first; res; res = res->next) {
    if (res->ordinal == ordinal) return res;
  }

  return NULL;
}


/*------------------------------------------------------------*/
at3d *
at3d_find_ordinal (mol3d *mol, int ordinal)
     /*
       Return the atom in the given molecule with the given ordinal.
      */
{
  res3d *res;
  at3d *at;

  /* pre */
  assert (mol);
  assert (ordinal >= 0);

  for (res = mol->first; res; res = res->next) {
    for (at = res->first; at; at = at->next) {
      if (at->ordinal == ordinal) return at;
    }
  }

  return NULL;
}


/*------------------------------------------------------------*/
void
mol3d_do_residues (mol3d *mol, void (*proc) (res3d *res))
     /*
       For all residues in the molecule, call the given procedure.
     */
{
  res3d *res;

  /* pre */
  assert (mol);
  assert (proc);

  for (res = mol->first; res; res = res->next) proc (res);
}


/*------------------------------------------------------------*/
void
mol3d_do_residues_all (mol3d *first_mol, void (*proc) (res3d *res))
     /*
       For all residues in the molecules in the linked list, call
       the given procedure.
     */
{
  mol3d *mol;
  res3d *res;

  /* pre */
  assert (first_mol);
  assert (proc);

  for (mol = first_mol; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) proc (res);
  }
}


/*------------------------------------------------------------*/
void
mol3d_do_atoms (mol3d *mol, void (*proc) (at3d *at))
     /*
       For all atoms in the molecule, call the given procedure.
     */
{
  res3d *res;
  at3d *at;

  /* pre */
  assert (mol);
  assert (proc);

  for (res = mol->first; res; res = res->next) {
    for (at = res->first; at; at = at->next) proc (at);
  }
}


/*------------------------------------------------------------*/
void
mol3d_do_atoms_all (mol3d *first_mol, void (*proc) (at3d *at))
     /*
       For all atoms in the molecules in the linked list, call
       the given procedure.
     */
{
  mol3d *mol;
  res3d *res;
  at3d *at;

  /* pre */
  assert (first_mol);
  assert (proc);

  for (mol = first_mol; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {
      for (at = res->first; at; at = at->next) proc (at);
    }
  }
}
