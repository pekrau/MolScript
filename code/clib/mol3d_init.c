/* mol3d_init

   Molecule coordinate data initialization routines.

   clib v1.1

   Copyright (C) 1998 Per Kraulis
     4-Jun-1998  split out of mol3d
*/

#include "mol3d_init.h"

/* public ====================
#include <mol3d.h>
==================== public */

#include <assert.h>
#include <ctype.h>

#include <str_utils.h>
#include <element_lookup.h>
#include <aa_lookup.h>


/*------------------------------------------------------------*/
void
mol3d_init (mol3d *mol, unsigned int flags)
     /*
       Initialize the molecule coordinate data according to the given
       combination of flags.
       Currently, MOL3D_INIT_SECSTRUC and MOL3D_INIT_ACCESS cannot
       be specified here; use the procedures in mol3d_secstruc and
       mol3d_access instead.
     */
{
  assert (mol);
  assert (! (MOL3D_INIT_SECSTRUC & flags));
  assert (! (MOL3D_INIT_ACCESS & flags));

  if ((flags & MOL3D_INIT_NOBLANKS) &&
      ! (mol->init & MOL3D_INIT_NOBLANKS)) mol3d_init_noblanks (mol);
  if ((flags & MOL3D_INIT_COLOURS) &&
      ! (mol->init & MOL3D_INIT_COLOURS)) mol3d_init_colours (mol);
  if ((flags & MOL3D_INIT_RADII) &&
      ! (mol->init & MOL3D_INIT_RADII)) mol3d_init_radii (mol);
  if ((flags & MOL3D_INIT_AACODES) &&
      ! (mol->init & MOL3D_INIT_AACODES)) mol3d_init_aacodes (mol);
  if ((flags & MOL3D_INIT_CENTRALS) &&
      ! (mol->init & MOL3D_INIT_CENTRALS)) mol3d_init_centrals_protein (mol);
  if ((flags & MOL3D_INIT_RESIDUE_ORDINALS) &&
      ! (mol->init & MOL3D_INIT_RESIDUE_ORDINALS)) {
    mol3d_init_residue_ordinals (mol, 1);
  }
  if ((flags & MOL3D_INIT_ATOM_ORDINALS) &&
      ! (mol->init & MOL3D_INIT_ATOM_ORDINALS)) {
    mol3d_init_atom_ordinals (mol, 1);
  }
  if ((flags & MOL3D_INIT_ELEMENTS) &&
      ! (mol->init & MOL3D_INIT_ELEMENTS)) mol3d_init_elements (mol);

  assert (flags & mol->init);
}


/*------------------------------------------------------------*/
void
mol3d_init_noblanks (mol3d *mol)
     /*
       Remove all blanks from strings: residue names and types,
       atom names, molecule secondary structure position data.
      */
{
  res3d *res;
  at3d *at;

  /* pre */
  assert (mol);

  for (res = mol->first; res; res = res->next) {
    str_remove_blanks (res->name);
    str_remove_blanks (res->type);
    for (at = res->first; at; at = at->next) str_remove_blanks (at->name);
  }

  mol->init |= MOL3D_INIT_NOBLANKS;
}


/*------------------------------------------------------------*/
void
mol3d_init_colours (mol3d *mol)
     /*
       Set standard residue and atom colours. Residue colours: grey 1.
       Atom colours are based on the first character of the atom name:
       C* grey 0.2, N* rgb 0 0 1, O* rgb 1 0 0, H* grey 1, S* rgb 1 1 0,
       P* rgb 1 0 1, others grey 0.8.
      */
{
  res3d *res;
  at3d *at;
  char *cp;

  /* pre */
  assert (mol);

  for (res = mol->first; res; res = res->next) {
    colour_set_grey (&(res->colour), 1.0);
    for (at = res->first; at; at = at->next) {
      for (cp = at->name; isspace (*cp); cp++) if (*cp == '\0') cp = "X";
      switch (*cp) {
      case 'C':
	colour_set_grey (&(at->colour), 0.2);
	break;
      case 'N':
	colour_set_rgb (&(at->colour), 0.0, 0.0, 1.0);
	break;
      case 'O':
	colour_set_rgb (&(at->colour), 1.0, 0.0, 0.0);
	break;
      case 'H':
	colour_set_grey (&(at->colour), 1.0);
	break;
      case 'S':
	colour_set_rgb (&(at->colour), 1.0, 1.0, 0.0);
	break;
      case 'P':
	colour_set_rgb (&(at->colour), 1.0, 0.0, 1.0);
	break;
      default:
	colour_set_grey (&(at->colour), 0.8);
      }
    }
  }

  mol->init |= MOL3D_INIT_COLOURS;
}


/*------------------------------------------------------------*/
void
mol3d_init_radii (mol3d *mol)
     /*
       Set the standard atom radii. These are based on he first
       character of the atom name: C* 1.7, N* 1.7, O* 1.35,
       H* 1.0, S* 1.8, P* 1.8, others 1.7.
      */
{
  res3d *res;
  at3d *at;
  char *cp;

  /* pre */
  assert (mol);

  for (res = mol->first; res; res = res->next) {
    for (at = res->first; at; at = at->next) {
      for (cp = at->name; isspace (*cp); cp++) if (*cp == '\0') cp = "X";
      switch (*cp) {
      case 'C':
	at->radius = 1.7;
	break;
      case 'N':
	at->radius = 1.7;
	break;
      case 'O':
	at->radius = 1.35;
	break;
      case 'H':
	at->radius = 1.0;
	break;
      case 'S':
	at->radius = 1.8;
	break;
      case 'P':
	at->radius = 1.8;
	break;
      default:
	at->radius = 1.7;
      }
    }
  }

  mol->init |= MOL3D_INIT_RADII;
}


/*------------------------------------------------------------*/
void
mol3d_init_aacodes (mol3d *mol)
     /*
       Set the amino-acid one-letter code field in the residues of
       the molecule interpreting the type field as a 3-letter code.
      */
{
  res3d *res;

  /* pre */
  assert (mol);

  for (res = mol->first; res; res = res->next) {
    res->code = aa_all3to1 (res->type);
  }

  mol->init |= MOL3D_INIT_AACODES;
}


/*------------------------------------------------------------*/
void
mol3d_init_centrals (mol3d *mol, const char *atomname)
     /*
       Set the atom with the given name as the central atom in each residue.
      */
{
  res3d *res;

  /* pre */
  assert (mol);
  assert (mol->init & MOL3D_INIT_NOBLANKS);
  assert (atomname);
  assert (*atomname);

  for (res = mol->first; res; res = res->next) {
    res->central = at3d_lookup (res, atomname);
  }

  mol->init |= MOL3D_INIT_CENTRALS;
}


/*------------------------------------------------------------*/
void
mol3d_init_centrals_protein (mol3d *mol)
     /*
       Set the CA atom as the central atom in each amino-acid residue
       in the molecule.
      */
{
  res3d *res;

  /* pre */
  assert (mol);
  assert (mol->init & MOL3D_INIT_NOBLANKS);
  assert (mol->init & MOL3D_INIT_AACODES);

  for (res = mol->first; res; res = res->next) {
    if (isalpha (res->code) && (res->code != 'X')) {
      res->central = at3d_lookup (res, "CA");
    } else {
      res->central = NULL;
    }
  }

  mol->init |= MOL3D_INIT_CENTRALS;
}


/*------------------------------------------------------------*/
void
mol3d_init_residue_ordinals (mol3d *mol, int start)
     /*
       Set the residue ordinals consecutively.
      */
{
  res3d *res;

  /* pre */
  assert (mol);

  for (res = mol->first; res; res = res->next) res->ordinal = start++;

  mol->init |= MOL3D_INIT_RESIDUE_ORDINALS;
}


/*------------------------------------------------------------*/
void
mol3d_init_residue_ordinals_protein (mol3d *mol)
     /*
       Set the residue ordinal values for all residues, starting at 1.
       The values are appropriate for proteins: Each new chain (sequence
       neighbour central atoms CA farther apart than 4.2 Angstrom)
       increases the ordinal value by 100000, and residues within a
       chain are numbered consecutively.
      */
{
  res3d *res;
  int count = 1;
  boolean chain = FALSE;

  /* pre */
  assert (mol);
  assert (mol->init & MOL3D_INIT_CENTRALS);

  for (res = mol->first; res; res = res->next) {
    if (chain) {
      if (res->central == NULL) {
	chain = FALSE;
	res->ordinal = 0;
      } else if (v3_distance (&(res->prev->central->xyz),
			      &(res->central->xyz)) > 4.2) {
	count = ((count / 100000) + 1) * 100000 + 1;
	res->ordinal = count++;
      } else {
	res->ordinal = count++;
      }
    } else {
      chain = res->central != NULL;
      if (chain) {
	count = ((count / 100000) + 1) * 100000 + 1;
	res->ordinal = count++;
      } else {
	res->ordinal = 0;
      }
    }
  }

  mol->init |= MOL3D_INIT_RESIDUE_ORDINALS;
}


/*------------------------------------------------------------*/
void
mol3d_init_atom_ordinals (mol3d *mol, int start)
     /*
       Set the atom ordinals consecutively.
      */
{
  res3d *res;
  at3d *at;

  /* pre */
  assert (mol);

  for (res = mol->first; res; res = res->next) {
    for (at = res->first; at; at = at->next) at->ordinal = start++;
  }

  mol->init |= MOL3D_INIT_ATOM_ORDINALS;
}


/*------------------------------------------------------------*/
void
mol3d_init_elements (mol3d *mol)
     /*
       Set the element field of all atoms in the molecule, according
       to the first characters of the atom name. If the first alphabetic
       character yields a valid element symbol, then this is used,
       otherwise a string containing the two first alphabetic characters
       is tested.
      */
{
  res3d *res;
  at3d *at;
  char symbol[3];
  int len, pos;

  /* pre */
  assert (mol);

  for (res = mol->first; res; res = res->next) {
    for (at = res->first; at; at = at->next) {
      len = 0;
      for (pos = 0; pos < 3; pos++) {
	if (len >= 2) {
	  at->element = 0;
	  break;
	}
	if (isalpha (at->name[pos])) {
	  symbol[len++] = at->name[pos];
	  symbol[len] = '\0';
	  at->element = element_number_convert (symbol);
	  if (at->element != 0) break;
	}
      }
    }
  }

  mol->init |= MOL3D_INIT_ELEMENTS;
}
