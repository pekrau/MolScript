/* mol3d_chain

   Find contiguous chains of residues in 3D molecules.

   clib v1.1

   Copyright (C) 1998 Per Kraulis
     8-Apr-1998  separated out from mol3d
    14-Apr-1998  modified for hgen
*/

#include "mol3d_chain.h"

#include <assert.h>
#include <stdlib.h>

/* public ====================
#include <mol3d.h>
#include <mol3d_init.h>

typedef struct s_mol3d_chain mol3d_chain;

struct s_mol3d_chain {
  int length;
  res3d **residues;
  at3d **atoms;
  mol3d_chain *next;
};
==================== public */


/*------------------------------------------------------------*/
mol3d_chain *
mol3d_chain_find (mol3d *first_mol, char *atomname,
		  double cutoff, int *res_sel)
     /*
       Find all chains in the molecules, consisting of contiguous residues
       with an atom having the given name. Successive atoms must be within
       the given distance cutoff. The residue selection is optional.
     */
{
  mol3d_chain *first = NULL;
  mol3d_chain *prev = NULL;
  mol3d_chain *curr = NULL;
  int curr_size;
  mol3d *mol;
  res3d *res;
  res3d *prevres = NULL;
  at3d *at;
  at3d *prevat = NULL;
  double sqcutoff;

  /* pre */
  assert (first_mol);
  assert (first_mol->init & MOL3D_INIT_NOBLANKS);
  assert (atomname);
  assert (*atomname);
  assert (cutoff > 0.0);

  sqcutoff = cutoff * cutoff;
  for (mol = first_mol; mol; mol = mol->next) {
    for (res = mol->first; res; res = res->next) {

      if ((res_sel != NULL) && /* residue selection is optional */
	  !(*res_sel++)) continue;

      at = at3d_lookup (res, atomname);
      if (at && prevat) {
	if (v3_close (&(prevat->xyz), &(at->xyz), sqcutoff)) {

	  if (curr == NULL) {
	    curr = malloc (sizeof (mol3d_chain));
	    curr_size = 128;
	    curr->residues = malloc (curr_size * sizeof (res3d *));
	    curr->atoms = malloc (curr_size * sizeof (at3d *));

	    curr->residues[0] = prevres;
	    curr->atoms[0] = prevat;
	    curr->length = 1;
	    curr->next = NULL;

	    if (prev) {
	      prev->next = curr;
	    } else {
	      first = curr;
	    }
	    prev = curr;

	  } else if (curr->length >= curr_size) {
	    curr_size *= 2;
	    curr->residues = realloc (curr->residues,
				      curr_size * sizeof (res3d *));
	    curr->atoms = realloc (curr->atoms, curr_size * sizeof (at3d *));
	  }

	  curr->residues[curr->length] = res;
	  curr->atoms[curr->length] = at;
	  curr->length++;

	} else {
	  curr = NULL;
	}
      } else {
	curr = NULL;
      }

      prevres = res;
      prevat = at;
    }
  }

  return first;
}


/*------------------------------------------------------------*/
void
mol3d_chain_delete (mol3d_chain *first_ch)
     /*
       Delete this chain and its next ones.
     */
{
  /* pre */
  assert (first_ch);

  if (first_ch->next) mol3d_chain_delete (first_ch->next);
  free (first_ch->residues);
  free (first_ch->atoms);
  free (first_ch);
}
