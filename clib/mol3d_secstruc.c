/* mol3d_secstruc

   Protein secondary structure determination from molecule coordinate data.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
    13-Mar-1997  first attempts
     5-Sep-1997  reasonably working hbonds implementation
     4-Jun-1998  moved out PDB data interpretation into mol3d_io

to do:
- check hbonds implementation
- allow residue list (NULL-ended array) as input
*/

#include "mol3d_secstruc.h"

/* public ====================
#include <mol3d.h>
==================== public */

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include <angle.h>
#include <str_utils.h>
#include <aa_lookup.h>


/*------------------------------------------------------------*/
void
mol3d_secstruc_initialize (mol3d *mol)
     /*
       Initialize the secstruc field (blank for amino-acid residues
       and dash '-' for others, based on the residue type) and the
       beta1, beta2 pointers for each residue in the molecule.
     */
{
  res3d *res;

  /* pre */
  assert (mol);

  for (res = mol->first; res; res = res->next) {
    if (is_amino_acid_type (res->type)) {
      res->secstruc = ' ';
    } else {
      res->secstruc = '-';
    }
    res->beta1 = NULL;
    res->beta2 = NULL;
  }
}


/*------------------------------------------------------------*/
void
mol3d_secstruc_ca_geom (mol3d *mol)
     /*
       Set the secondary structure using an algorithm based on inter-CA
       distances and torsion angles.
     */
{
  at3d **atoms, *at;
  int ca_count = 0;
  res3d *res;
  int slot, slot2;
  double angle, dist;

  /* pre */
  assert (mol);
  assert (mol->init & MOL3D_INIT_NOBLANKS);
  assert (mol->init & MOL3D_INIT_AACODES);

  mol3d_secstruc_initialize (mol);

  atoms = malloc (mol3d_count_residues (mol) * sizeof (at3d *));

  for (res = mol->first; res; res = res->next) {
    if (res->code != 'X') {
      at = at3d_lookup (res, "CA");
      if (at) atoms[ca_count++] = at;
    }
  }
				/* find helices */
  for (slot = 0; slot < ca_count - 3; slot++) {
    if (v3_distance (&(atoms[slot]->xyz),
		     &(atoms[slot+1]->xyz)) > 4.2) continue;
    if (v3_distance (&(atoms[slot+1]->xyz),
		     &(atoms[slot+2]->xyz)) > 4.2) continue;
    if (v3_distance (&(atoms[slot+2]->xyz),
		     &(atoms[slot+3]->xyz)) > 4.2) continue;

    angle = to_degrees (v3_torsion_points (&(atoms[slot]->xyz),
					   &(atoms[slot+1]->xyz),
					   &(atoms[slot+2]->xyz),
					   &(atoms[slot+3]->xyz)));

    dist = v3_distance (&(atoms[slot]->xyz), &(atoms[slot+3]->xyz));

    if ((angle > -60.0) && (angle < -46.0) && (dist < 5.9)) {
      atoms[slot]->res->secstruc = 'H';
      atoms[slot+1]->res->secstruc = 'H';
      atoms[slot+2]->res->secstruc = 'H';
      atoms[slot+3]->res->secstruc = 'H';
    }
  }
				/* find parallel strands */
  for (slot = 0; slot < ca_count - 2; slot++) {
    if (atoms[slot]->res->secstruc == 'H') continue;
    if (atoms[slot+1]->res->secstruc == 'H') continue;
    if (atoms[slot+2]->res->secstruc == 'H') continue;

    if (v3_distance (&(atoms[slot]->xyz),
		     &(atoms[slot+1]->xyz)) > 4.2) continue;
    if (v3_distance (&(atoms[slot+1]->xyz),
		     &(atoms[slot+2]->xyz)) > 4.2) continue;

    for (slot2 = slot + 6; slot2 < ca_count - 2; slot2++) {
      if (atoms[slot2]->res->secstruc == 'H') continue;
      if (atoms[slot2+1]->res->secstruc == 'H') continue;
      if (atoms[slot2+2]->res->secstruc == 'H') continue;

      if (v3_distance (&(atoms[slot]->xyz),
		       &(atoms[slot2]->xyz)) > 5.8) continue;
      if (v3_distance (&(atoms[slot+1]->xyz),
		       &(atoms[slot2+1]->xyz)) > 5.8) continue;
      if (v3_distance (&(atoms[slot+2]->xyz),
		       &(atoms[slot2+2]->xyz)) > 5.8) continue;

      atoms[slot]->res->secstruc = 'E';
      atoms[slot+1]->res->secstruc = 'E';
      atoms[slot+2]->res->secstruc = 'E';
      atoms[slot2]->res->secstruc = 'E';
      atoms[slot2+1]->res->secstruc = 'E';
      atoms[slot2+2]->res->secstruc = 'E';
    }
  }

				/* find anti-parallel strands */
  for (slot = 0; slot < ca_count - 2; slot++) {
    if (atoms[slot]->res->secstruc == 'H') continue;
    if (atoms[slot+1]->res->secstruc == 'H') continue;
    if (atoms[slot+2]->res->secstruc == 'H') continue;

    if (v3_distance (&(atoms[slot]->xyz),
		     &(atoms[slot+1]->xyz)) > 4.2) continue;
    if (v3_distance (&(atoms[slot+1]->xyz),
		     &(atoms[slot+2]->xyz)) > 4.2) continue;

    for (slot2 = slot + 7; slot2 < ca_count; slot2++) {
      if (atoms[slot2]->res->secstruc == 'H') continue;
      if (atoms[slot2-1]->res->secstruc == 'H') continue;
      if (atoms[slot2-2]->res->secstruc == 'H') continue;

      if (v3_distance (&(atoms[slot2]->xyz),
		       &(atoms[slot2-1]->xyz)) > 4.2) continue;
      if (v3_distance (&(atoms[slot2-1]->xyz),
		       &(atoms[slot2-2]->xyz)) > 4.2) continue;

      if (v3_distance (&(atoms[slot]->xyz),
		       &(atoms[slot2]->xyz)) > 5.7) continue;
      if (v3_distance (&(atoms[slot+1]->xyz),
		       &(atoms[slot2-1]->xyz)) > 5.7) continue;
      if (v3_distance (&(atoms[slot+2]->xyz),
		       &(atoms[slot2-2]->xyz)) > 5.7) continue;

      atoms[slot]->res->secstruc = 'E';
      atoms[slot+1]->res->secstruc = 'E';
      atoms[slot+2]->res->secstruc = 'E';
      atoms[slot2]->res->secstruc = 'E';
      atoms[slot2-1]->res->secstruc = 'E';
      atoms[slot2-2]->res->secstruc = 'E';
    }
  }

  free (atoms);
}


/*============================================================*/
typedef struct s_hbonds_record hbonds_record;

struct s_hbonds_record {
  res3d *res;
  vector3 *ca, *n, *c, *o;
  vector3 h;
  hbonds_record *co_hbond, *hn_hbond;
  double co_energy, hn_energy;
  unsigned int pattern;
};

#define HBONDS_3TURN    0x00000001
#define HBONDS_4TURN    0x00000002
#define HBONDS_5TURN    0x00000004
#define HBONDS_ANTIPARA 0x00000008
#define HBONDS_PARA     0x00000010


/*------------------------------------------------------------*/
boolean
mol3d_secstruc_hbonds (mol3d *mol)
     /*
       Determine the secondary structure from the backbone coordinates
       using a DSSP-like algorithm, which is based on peptide hydrogen
       bond patterns. The central atoms for protein
       (mol3d_init_centrals_protein), and the residue ordinals for
       protein (mol3d_init_residue_ordinals_protein) should have been set.
       Return FALSE if backbone coordinates are missing.
     */
{
  res3d *res;
  at3d *at;
  int count, slot, swap, diff1, diff2;
  hbonds_record *records;
  hbonds_record *rec1, *rec2, *rec3;
  double energy;
  char secstruc;

  /* pre */
  assert (mol);
  assert (mol->init & MOL3D_INIT_NOBLANKS);
  assert (mol->init & MOL3D_INIT_AACODES);
  assert (mol->init & MOL3D_INIT_CENTRALS);
  assert (mol->init & MOL3D_INIT_RESIDUE_ORDINALS);

  mol3d_secstruc_initialize (mol);

  count = 0;
  for (res = mol->first; res; res = res->next) if (res->code != 'X') count++;
  records = calloc (count + 3, sizeof (hbonds_record));

  rec1 = records;
  for (res = mol->first; res; res = res->next) {
    if (res->code == 'X') continue;

    at = at3d_lookup (res, "CA");
    if (at == NULL) continue;
    rec1->ca = &(at->xyz);
    at = at3d_lookup (res, "N");
    if (at == NULL) {
      rec1->ca = NULL;
      continue;
    }
    rec1->n = &(at->xyz);
    at = at3d_lookup (res, "C");
    if (at == NULL) {
      rec1->ca = NULL;
      continue;
    }
    rec1->c = &(at->xyz);
    at = at3d_lookup (res, "O");
    if (at == NULL) {
      rec1->ca = NULL;
      continue;
    }
    rec1->o = &(at->xyz);

    rec1->res = res;		/* other fields initialized by calloc */
    rec1->co_energy = 1.0e10;
    rec1->hn_energy = 1.0e10;

    rec1++;
  }

  if (rec1 == records) {	/* apparently only CA coordinates present */
    free (records);
    return FALSE;
  }
				/* H position */
  for (rec1 = records, rec2 = rec1 + 1; rec2->ca; rec1++, rec2++) {
    if (v3_distance (rec1->c, rec2->n) <= 2.0) {
      v3_difference (&(rec2->h), rec1->c, rec1->o);
    } else {
      v3_difference (&(rec2->h), rec2->o, rec2->c);
    }
    v3_normalize (&(rec2->h));
    v3_scale (&(rec2->h), 1.008);
    v3_add (&(rec2->h), rec2->n);
  }

  for (rec1 = records; rec1->ca; rec1++) { /* this is inefficient; use grid? */
    for (rec2 = rec1 + 3; rec2->ca; rec2++) {
      if (v3_distance (rec1->ca, rec2->ca) > 8.0) continue;

      energy = 0.42 * 0.20 * 332.0 *
	(1.0 / v3_distance (rec1->o, rec2->n) +
	 1.0 / v3_distance (rec1->c, &(rec2->h)) -
	 1.0 / v3_distance (rec1->o, &(rec2->h)) -
	 1.0 / v3_distance (rec1->c, rec2->n));
      if (energy < -0.5) {
	if (energy < rec1->co_energy) {
	  rec1->co_hbond = rec2;
	  rec1->co_energy = energy;
	  rec2->hn_hbond = rec1;
	  rec2->hn_energy = energy;
	}
      }

      energy = 0.42 * 0.20 * 332.0 *
	(1.0 / v3_distance (rec1->n, rec2->o) +
	 1.0 / v3_distance (&(rec1->h), rec2->c) -
	 1.0 / v3_distance (&(rec1->h), rec2->o) -
	 1.0 / v3_distance (rec1->n, rec2->c));
      if (energy < -0.5) {
	if (energy < rec1->hn_energy) {
	  rec1->hn_hbond = rec2;
	  rec1->hn_energy = energy;
	  rec2->co_hbond = rec1;
	  rec2->co_energy = energy;
	}
      }
    }
  }

  for (rec1 = records; rec1->ca; rec1++) { /* N-turns; 3, 4, 5 */
    if (rec1->co_hbond == NULL) continue;

    count = rec1->co_hbond->res->ordinal - rec1->res->ordinal;
    switch (count) {
    case 3:
      rec1->pattern |= HBONDS_3TURN;
      break;
    case 4:
      rec1->pattern |= HBONDS_4TURN;
      break;
    case 5:
      rec1->pattern |= HBONDS_5TURN;
      break;
    default:
      break;
    }
  }

  for (rec1 = records; rec1->ca; rec1++) { /* anti-parallel bridge 1 */
    rec2 = rec1->co_hbond;
    if (rec2 == NULL) continue;

    if (rec2->co_hbond == rec1) {
      rec1->pattern |= HBONDS_ANTIPARA;
      if (rec1->res->beta1 == NULL) {
	rec1->res->beta1 = rec2->res;
      } else if (rec1->res->beta1 != rec2->res) {
	rec1->res->beta2 = rec2->res;
      }
      rec2->pattern |= HBONDS_ANTIPARA;
      if (rec2->res->beta1 == NULL) {
	rec2->res->beta1 = rec1->res;
      } else if (rec2->res->beta1 != rec1->res) {
	rec2->res->beta2 = rec1->res;
      }
    }
  }

  for (rec1 = records; rec1->ca; rec1++) { /* anti-parallel bridge 2 */
    if (rec1->co_hbond == NULL) continue;

    rec2 = (rec1 + 2)->hn_hbond;
    if (rec2 == NULL) continue;

    if (rec1->co_hbond->res->ordinal - rec2->res->ordinal == 2) {
      rec2++;
      rec3 = rec1 + 1;
      rec3->pattern |= HBONDS_ANTIPARA;
      if (rec3->res->beta1 == NULL) {
	rec3->res->beta1 = rec2->res;
      } else if (rec3->res->beta1 != rec2->res) {
	rec3->res->beta2 = rec2->res;
      }
      rec2->pattern |= HBONDS_ANTIPARA;
      if (rec2->res->beta1 == NULL) {
	rec2->res->beta1 = rec3->res;
      } else if (rec2->res->beta1 != rec3->res) {
	rec2->res->beta2 = rec3->res;
      }
    }
  }

  for (rec1 = records + 1; (rec1 + 1)->ca; rec1++) { /* parallel bridge 1 */
    rec2 = (rec1 - 1)->co_hbond;
    if (rec2 == NULL) continue;
    if (rec2->co_hbond == NULL) continue;

    if (rec2->co_hbond->res->ordinal - rec1->res->ordinal == 1) {
      rec1->pattern |= HBONDS_PARA;
      if (rec1->res->beta1 == NULL) {
	rec1->res->beta1 = rec2->res;
      } else if (rec1->res->beta1 != rec2->res) {
	rec1->res->beta2 = rec2->res;
      }
      rec2->pattern |= HBONDS_PARA;
      if (rec2->res->beta1 == NULL) {
	rec2->res->beta1 = rec1->res;
      } else if (rec2->res->beta1 != rec1->res) {
	rec2->res->beta2 = rec1->res;
      }
    }
  }

  for (rec1 = records; rec1->ca; rec1++) { /* parallel bridge 2 */
    if (rec1->hn_hbond == NULL) continue;
    if (rec1->co_hbond == NULL) continue;

    if (rec1->co_hbond->res->ordinal - rec1->hn_hbond->res->ordinal == 2) {
      rec2 = rec1->hn_hbond + 1;
      rec1->pattern |= HBONDS_PARA;
      if (rec1->res->beta1 == NULL) {
	rec1->res->beta1 = rec2->res;
      } else if (rec1->res->beta1 != rec2->res) {
	rec1->res->beta2 = rec2->res;
      }
      rec2->pattern |= HBONDS_PARA; 
      if (rec2->res->beta1 == NULL) {
	rec2->res->beta1 = rec1->res;
      } else if (rec2->res->beta1 != rec1->res) {
	rec2->res->beta2 = rec1->res;
      }
    }
  }
				/* swap beta1/beta2 for sheet coherence */
  for (rec1 = records + 1; rec1->ca; rec1++) {
    if (! rec1->res->beta1) continue;

    swap = FALSE;
    rec2 = rec1 - 1;
    if (rec2->res->beta1) {
      diff1 = abs (rec2->res->beta1->ordinal - rec1->res->beta1->ordinal);
      swap = diff1 > 2;
    } else if (rec2->res->beta2) {
      diff1 = abs (rec2->res->beta2->ordinal - rec1->res->beta1->ordinal);
      swap = diff1 <= 2;
    } else if (rec1 > records + 1) {
      rec2 = rec1 - 2;
      if (rec2->res->beta1) {
	diff1 = abs (rec2->res->beta1->ordinal - rec1->res->beta1->ordinal);
	swap = diff1 > 2;
      } else if (rec2->res->beta2) {
	diff1 = abs (rec2->res->beta2->ordinal - rec1->res->beta1->ordinal);
	swap = diff1 <= 2;
      }
    }
    if (swap) {
      res = rec1->res->beta1;
      rec1->res->beta1 = rec1->res->beta2;
      rec1->res->beta2 = res;
    }
  }

  secstruc = 'h';		/* 4-helix */
  for (rec1 = records; (rec1 + 1)->ca; rec1++) {
    rec2 = rec1 + 1;
    if ((rec1->pattern & HBONDS_4TURN) && (rec2->pattern & HBONDS_4TURN)) {
      rec2->res->secstruc = secstruc;
      secstruc = toupper (secstruc);
      (++rec2)->res->secstruc = secstruc;
      (++rec2)->res->secstruc = secstruc;
      (++rec2)->res->secstruc = secstruc;
    } else {
      secstruc = 'h';
    }
  }
				/* strand, beta1 pass */
  for (rec1 = records; rec1->ca; rec1++) {
    if (rec1->res->beta1 == NULL) continue;

    secstruc = 'e';
    for (rec2 = rec1 + 1; rec2->ca; rec2++) {
      if (rec2->res->beta1) {
	diff2 = 2;
      } else {
	rec2++;
	if (rec2->ca == NULL) break;
	if (rec2->res->beta1) {
	  diff2 = 3;
	} else {
	  rec2++;
	  if (rec2->ca == NULL) break;
	  if (rec2->res->beta1 == NULL) break;
	  diff2 = 2;
	}
      }
      diff1 = abs (rec1->res->beta1->ordinal - rec2->res->beta1->ordinal);
      if (diff1 <= diff2) {
	for (rec3 = rec1; rec3 <= rec2; rec3++) {
	  switch (rec3->res->secstruc) {
	  case ' ':
	  case 'e':
	    rec3->res->secstruc = secstruc;
	  case 'E':
	    secstruc = 'E';
	    break;
	  default:
	    break;
	  }
	}
      }
      rec1 = rec2;
    }
  }
				/* strand, beta2 pass */
  for (rec1 = records; rec1->ca; rec1++) {
    if (rec1->res->beta2 == NULL) continue;

    secstruc = 'e';
    for (rec2 = rec1 + 1; rec2->ca; rec2++) {
      if (rec2->res->beta2) {
	diff2 = 2;
      } else {
	rec2++;
	if (rec2->ca == NULL) break;
	if (rec2->res->beta2) {
	  diff2 = 3;
	} else {
	  rec2++;
	  if (rec2->ca == NULL) break;
	  if (rec2->res->beta2 == NULL) break;
	  diff2 = 2;
	}
      }
      diff1 = abs (rec1->res->beta2->ordinal - rec2->res->beta2->ordinal);
      if (diff1 <= diff2) {
	for (rec3 = rec1; rec3 <= rec2; rec3++) {
	  switch (rec3->res->secstruc) {
	  case ' ':
	  case 'e':
	    rec3->res->secstruc = secstruc;
	  case 'E':
	    secstruc = 'E';
	    break;
	  default:
	    break;
	  }
	}
      }
      rec1 = rec2;
    }
  }

  secstruc = 'i';		/* 5-helix */
  for (rec1 = records; (rec1 + 1)->ca; rec1++) {
    if ((rec1->pattern & HBONDS_5TURN) &&
	((rec1 + 1)->pattern & HBONDS_5TURN)) {
      for (slot = 1; slot <= 5; slot++) {
	rec2 = rec1 + slot;
	if (rec2->res->secstruc == ' ') {
	  rec2->res->secstruc = secstruc;
	  secstruc = toupper (secstruc);
	}
      }
    } else {
      secstruc = 'i';
    }
  }

  secstruc = 'g';		/* 3-helix */
  for (rec1 = records; (rec1 + 1)->ca; rec1++) {
    if ((rec1->pattern & HBONDS_3TURN) &&
	((rec1 + 1)->pattern & HBONDS_3TURN)) {
      for (slot = 1; slot <= 3; slot++) {
	rec2 = rec1 + slot;
	if (rec2->res->secstruc == ' ') {
	  rec2->res->secstruc = secstruc;
	  secstruc = toupper (secstruc);
	}
      }
    } else {
      secstruc = 'g';
    }
  }
				/* convert singlet 'G' or 'I' into 't' */
  for (rec1 = records; rec1->ca; rec1++) {
    switch (rec1->res->secstruc) {
    case 'g':
    case 'G':
    case 'i':
    case 'I':
      secstruc = toupper (rec1->res->secstruc);
      if (rec1 > records) {
	swap = secstruc != toupper ((rec1 - 1)->res->secstruc);
      } else {
	swap = TRUE;
      }
      if ((rec1 + 1)->ca) {
	swap = swap && (secstruc != toupper ((rec1 + 1)->res->secstruc));
      }
      if (swap) rec1->res->secstruc = 't';
      break;
    default:
      break;
    }
  }
				/* single 5-turns */
  for (rec1 = records; rec1->ca; rec1++) {
    if (rec1->pattern & HBONDS_5TURN) {
      if (rec1 > records) {
	swap = ! ((rec1 - 1)->pattern & HBONDS_5TURN);
      } else {
	swap = TRUE;
      }
      swap = swap && (! ((rec1 + 1)->pattern & HBONDS_5TURN));
      if (swap) {
	secstruc = 't';
	for (slot = 1; slot <= 5; slot++) {
	  rec2 = rec1 + slot;
	  if (rec2->res->secstruc == ' ') {
	    rec2->res->secstruc = secstruc;
	    secstruc = toupper (secstruc);
	  }
	}
      }
    }
  }
				/* single 4-turns */
  for (rec1 = records; rec1->ca; rec1++) {
    if (rec1->pattern & HBONDS_4TURN) {
      if (rec1 > records) {
	swap = ! ((rec1 - 1)->pattern & HBONDS_4TURN);
      } else {
	swap = TRUE;
      }
      swap = swap && (! ((rec1 + 1)->pattern & HBONDS_4TURN));
      if (swap) {
	secstruc = 't';
	for (slot = 1; slot <= 4; slot++) {
	  rec2 = rec1 + slot;
	  if (rec2->res->secstruc == ' ') {
	    rec2->res->secstruc = secstruc;
	    secstruc = toupper (secstruc);
	  }
	}
      }
    }
  }
				/* single 3-turns */
  for (rec1 = records; rec1->ca; rec1++) {
    if (rec1->pattern & HBONDS_3TURN) {
      if (rec1 > records) {
	swap = ! ((rec1 - 1)->pattern & HBONDS_3TURN);
      } else {
	swap = TRUE;
      }
      swap = swap && (! ((rec1 + 1)->pattern & HBONDS_3TURN));
      if (swap) {
	secstruc = 't';
	for (slot = 1; slot <= 3; slot++) {
	  rec2 = rec1 + slot;
	  if (rec2->res->secstruc == ' ') {
	    rec2->res->secstruc = secstruc;
	    secstruc = toupper (secstruc);
	  }
	}
      }
    }
  }

  /* debug printout
  for (rec1 = records; rec1->ca; rec1++) {
    printf ("%4s %s %c", rec1->res->name, rec1->res->type, rec1->res->secstruc);
    if (rec1->pattern & HBONDS_3TURN) printf (" 3");
    if (rec1->pattern & HBONDS_4TURN) printf (" 4");
    if (rec1->pattern & HBONDS_5TURN) printf (" 5");
    if (rec1->pattern & HBONDS_ANTIPARA) {
      printf (" a");
    } else {
      printf ("  ");
    }
    if (rec1->pattern & HBONDS_PARA) {
      printf (" p");
    } else {
      printf ("  ");
    }
    if (rec1->res->beta1) {
      printf (" %4s", rec1->res->beta1->name);
    } else {
      printf ("     ");
    }
    if (rec1->res->beta2) {
      printf (" %4s", rec1->res->beta2->name);
    } else {
      printf ("     ");
    }
    printf ("\n");
  }
  */

  free (records);

  return TRUE;
}
