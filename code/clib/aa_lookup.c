/* aa_lookup

   Amino-acid residue data lookup; 1 and 3 letter codes, integer code,
   relative abundances (in Swissprot Aug 1997), Fauchere-Pliska and
   Kyte-Dolittle hydrophobic scales, and basic binary properties.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
    13-Feb-1997  fairly finished
    10-May-1997  added res_aa_int
     5-Nov-1997  added 'I' for inosine nucleotide type, as in PDB
    23-Feb-1998  normalized property functions
     9-Mar-1998  added binary properties
    15-Apr-1998  for hgen
*/

#include "aa_lookup.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* public ====================
#include <boolean.h>
==================== public */


/*============================================================*/
typedef struct {
  char *c3;
  char c1;
  int number;
  double abundance;
  double fauchere_pliska;
  double kyte_doolittle;
  unsigned int property;
} aa_node;

#define CONF_SPECIAL    0x00000001
#define RING            0x00000002
#define HBOND_DONOR     0x00000004
#define HBOND_ACCEPTOR  0x00000008
#define POLAR           0x00000010
#define AROMATIC        0x00000020
#define POSITIVE_CHARGE 0x00000040
#define NEGATIVE_CHARGE 0x00000080
#define ZN_BINDING      0x00000100


/*============================================================*/
static aa_node aa_nodes[] =
{
  {"ALA", 'A', 0,  0.0756,  1.52,  1.8,
   NULL },
  {"ARG", 'R', 1,  0.0515,  2.84, -4.5,
   HBOND_DONOR | POLAR | POSITIVE_CHARGE },
  {"ASN", 'N', 2,  0.0451,  2.41, -3.5,
   HBOND_DONOR | HBOND_ACCEPTOR | POLAR },
  {"ASP", 'D', 3,  0.0530,  2.60, -3.5,
   HBOND_ACCEPTOR | POLAR | NEGATIVE_CHARGE },
  {"CYS", 'C', 4,  0.0168,  1.70,  2.5,
   ZN_BINDING },
  {"GLN", 'Q', 5,  0.0401,  2.05, -3.5,
   HBOND_DONOR | HBOND_ACCEPTOR | POLAR },
  {"GLU", 'E', 6,  0.0635,  2.47, -3.5,
   HBOND_ACCEPTOR | POLAR | NEGATIVE_CHARGE },
  {"GLY", 'G', 7,  0.0683,  1.83, -0.4,
   CONF_SPECIAL },
  {"HIS", 'H', 8,  0.0224,  1.70, -3.2,
   HBOND_DONOR | HBOND_ACCEPTOR | POLAR | POSITIVE_CHARGE | ZN_BINDING},
  {"ILE", 'I', 9,  0.0578,  0.03,  4.5,
   NULL },
  {"LEU", 'L', 10, 0.0940,  0.13,  3.8,
   NULL },
  {"LYS", 'K', 11, 0.0596,  2.82, -3.9,
   HBOND_DONOR | POLAR | POSITIVE_CHARGE },
  {"MET", 'M', 12, 0.0236,  0.60,  1.9,
   ZN_BINDING },
  {"PHE", 'F', 13, 0.0409,  0.04,  2.8,
   RING | AROMATIC },
  {"PRO", 'P', 14, 0.0491,  1.34, -1.6,
   RING | CONF_SPECIAL },
  {"SER", 'S', 15, 0.0718,  1.87, -0.8,
   HBOND_DONOR | HBOND_ACCEPTOR | POLAR },
  {"THR", 'T', 16, 0.0571,  1.57, -0.7,
   HBOND_DONOR | HBOND_ACCEPTOR | POLAR },
  {"TRP", 'W', 17, 0.0124, -0.42, -0.9,
   RING | HBOND_DONOR | AROMATIC },
  {"TYR", 'Y', 18, 0.0319,  0.87, -1.3,
   RING | HBOND_DONOR | HBOND_ACCEPTOR | POLAR | AROMATIC }, 
  {"VAL", 'V', 19, 0.0655,  0.61,  4.2,
   NULL }
};
static char aa_c1_codes[] = "ARNDCQEGHILKMFPSTWYV";
static int aa_nodes_count = 20;

static const double fauchere_pliska_max = 2.84;
static const double fauchere_pliska_min = -0.42;
static const double kyte_doolittle_min = -4.5;
static const double kyte_doolittle_max = 4.5;

static aa_node aa_nonstd_nodes[] =
{
  {"ASX", 'B', -1},
  {"CPR", 'P', 14},
  {"CSH", 'C', 4},
  {"CSM", 'C', 4},
  {"CYH", 'C', 4},
  {"GLX", 'Z', -1},
  {"TRY", 'W', 17}
};
static int aa_nonstd_nodes_count = 7;

static aa_node water_nodes[] =
{
  {"H2O", 'X', -1},
  {"HHO", 'X', -1},
  {"HOH", 'X', -1},
  {"OH2", 'X', -1},
  {"OHH", 'X', -1},
  {"SOL", 'X', -1},
  {"WAT", 'X', -1}
};
static int water_nodes_count = 7;


/*------------------------------------------------------------*/
static int
aa_node_c3_compare (const aa_node *n1, const aa_node *n2)
{
  return strncmp (n1->c3, n2->c3, 3);
}


/*------------------------------------------------------------*/
int
aa_ordinal (char *c3)
     /*
       Return the ordinal for the amino-acid 3-character code.
       Return -1 if the code is invalid.
     */
{
  aa_node key;
  aa_node *n;

  /* pre */
  assert (c3);
  assert (*c3);

  while (isspace (*c3)) if (*c3++ == '\0') return -1;
  key.c3 = c3;
  n = bsearch (&key, aa_nodes, aa_nodes_count, sizeof (aa_node),
	       (int(*) (const void *, const void *)) aa_node_c3_compare);
  if (n) {
    return n->number;
  } else {
    return -1;
  }
}


/*------------------------------------------------------------*/
char
aa_3to1 (char *c3)
     /*
       Return the 1-character code for the standard amino-acid 3-character
       code. Return 'X' if the code is invalid.
     */
{
  aa_node key;
  aa_node *n;

  /* pre */
  assert (c3);
  assert (*c3);

  while (isspace (*c3)) if (*c3++ == '\0') return 'X';
  key.c3 = c3;
  n = bsearch (&key, aa_nodes, aa_nodes_count, sizeof (aa_node),
	       (int(*) (const void *, const void *)) aa_node_c3_compare);
  if (n) {
    return n->c1;
  } else {
    return 'X';
  }
}


/*------------------------------------------------------------*/
char *
aa_1to3 (char c1)
     /*
       Return a pointer to the 3-character code for the 1-character code.
       If the character is invalid, return NULL.
     */
{
  char *p = strchr (aa_c1_codes, toupper (c1));
  if (p) {
    int slot = p - aa_c1_codes;
    return aa_nodes[slot].c3;
  } else {
    return NULL;
  }
}


/*------------------------------------------------------------*/
char
aa_all3to1 (char *c3)
     /*
       Return the 1-character code for the amino-acid 3-character code,
       including some non-standard codes.
       Return 'X' if the code is invalid.
     */
{
  char c1;

  /* pre */
  assert (c3);
  assert (*c3);

  c1 = aa_3to1 (c3);
  if (c1 != 'X') {
    return c1;
  } else {
    aa_node key;
    aa_node *n;
    while (isspace (*c3)) if (*c3++ == '\0') return 'X';
    key.c3 = c3;
    n = bsearch (&key, aa_nonstd_nodes, aa_nonstd_nodes_count, sizeof(aa_node),
		 (int(*) (const void *, const void *)) aa_node_c3_compare);
    if (n) {
      return n->c1;
    } else {
      return 'X';
    }
  }
}


/*------------------------------------------------------------*/
int
aa_int (char c1)
     /*
       Return the ordinal for the amino-acid 1-character code.
       Return -1 if the character is invalid.
     */
{
  switch (toupper (c1)) {	/* this depends on the order in aa_c1_codes */
  case 'A':
    return 0;
  case 'R':
    return 1;
  case 'N':
    return 2;
  case 'D':
    return 3;
  case 'C':
    return 4;
  case 'Q':
    return 5;
  case 'E':
    return 6;
  case 'G':
    return 7;
  case 'H':
    return 8;
  case 'I':
    return 9;
  case 'L':
    return 10;
  case 'K':
    return 11;
  case 'M':
    return 12;
  case 'F':
    return 13;
  case 'P':
    return 14;
  case 'S':
    return 15;
  case 'T':
    return 16;
  case 'W':
    return 17;
  case 'Y':
    return 18;
  case 'V':
    return 19;
  default:
    return -1;
  }
}


/*------------------------------------------------------------*/
char
aa_code (int aa_int)
     /*
       Return the 1-character code for the amino-acid ordinal.
       Return 'X' if the ordinal is invalid.
     */
{
  if (aa_int < 0) return 'X';
  if (aa_int > 19) return 'X';
  return aa_c1_codes[aa_int];
}


/*------------------------------------------------------------*/
boolean
is_amino_acid_type (char *c3)
     /*
       Is the 3-character code an amino-acid type?
     */
{
  /* pre */
  assert (c3);
  assert (*c3);

  return ('X' != aa_all3to1 (c3));
}


/*------------------------------------------------------------*/
boolean
is_water_type (char *c3)
     /*
       Is the 3-character code a typical water type, as in the PDB?
     */
{
  aa_node key;

  /* pre */
  assert (c3);
  assert (*c3);

  while (isspace (*c3)) if (*c3++ == '\0') return FALSE;
  key.c3 = c3;
  return (NULL !=
	  bsearch (&key, water_nodes, water_nodes_count, sizeof (aa_node),
		   (int(*) (const void *, const void *)) aa_node_c3_compare));
}


/*------------------------------------------------------------*/
boolean
is_nucleic_acid_type (char *c3)
     /*
       Is the 3-character code a typical nucleotide, as in the PDB?
     */
{
  /* pre */
  assert (c3);
  assert (*c3);

  while (isspace (*c3)) if (*c3++ == '\0') return FALSE;
  if (*c3 == '+') c3++;		/* modified base according to PDB convention */

  switch (*c3) {
  case 'A':
  case 'C':
  case 'I':
  case 'G':
  case 'T':
  case 'U':
    c3++;
    return ((*c3 == '\0') || (*c3 == ' '));
  default:
    return FALSE;
  }
}


/*------------------------------------------------------------*/
double
aa_abundance (int aa_int)
     /*
       Return the relative abundance of the amino-acid, as in Swissprot.
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].abundance;
}


/*------------------------------------------------------------*/
double
aa_fauchere_pliska (int aa_int)
     /*
       Return the Fauchere-Pliska hydrophobicity value.
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].fauchere_pliska;
}


/*------------------------------------------------------------*/
double
aa_fauchere_pliska_normalized (int aa_int)
     /*
       Return the Fauchere-Pliska hydrophobicity value, normalized to
       the range 0.0 (hydrophobic) to 1.0 (hydrophilic).
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return (aa_nodes[aa_int].fauchere_pliska - fauchere_pliska_min) /
    (fauchere_pliska_max - fauchere_pliska_min);
}


/*------------------------------------------------------------*/
double
aa_kyte_doolittle (int aa_int)
     /*
       Return the Kyte-Doolittle hydrophobicity value.
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].kyte_doolittle;
}


/*------------------------------------------------------------*/
double
aa_kyte_doolittle_normalized (int aa_int)
     /*
       Return the Kyte-Doolittle hydrophobicity value, normalized to
       the range 0.0 (hydrophilic) to 1.0 (hydrophobic).
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return (aa_nodes[aa_int].kyte_doolittle - kyte_doolittle_min) /
    (kyte_doolittle_max - kyte_doolittle_min);
}


/*------------------------------------------------------------*/
boolean
aa_conformation_special (int aa_int)
     /*
       Does the amino-acid have special backbone conformational properties?
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].property & CONF_SPECIAL;
}


/*------------------------------------------------------------*/
boolean
aa_ring (int aa_int)
     /*
       Does the amino-acid structure contain a ring?
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].property & RING;
}

/*------------------------------------------------------------*/
boolean
aa_hbond_forming (int aa_int)
     /*
       Can the amino-acid sidechain participate in hydrogen bonds?
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].property & (HBOND_DONOR | HBOND_ACCEPTOR);
}


/*------------------------------------------------------------*/
boolean
aa_hbond_donor (int aa_int)
     /*
       Is the amino-acid sidechain a hydrogen bond donor?
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].property & HBOND_DONOR;
}


/*------------------------------------------------------------*/
boolean
aa_hbond_acceptor (int aa_int)
     /*
       Is the amino-acid sidechain a hydrogen bond acceptor?
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].property & HBOND_ACCEPTOR;
}


/*------------------------------------------------------------*/
boolean
aa_polar (int aa_int)
     /*
       Is the amino-acid polar?
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].property & POLAR;
}


/*------------------------------------------------------------*/
boolean
aa_aromatic (int aa_int)
     /*
       Is the amino-acid aromatic?
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].property & AROMATIC;
}


/*------------------------------------------------------------*/
boolean
aa_positive_charge (int aa_int)
     /*
       Does the amino-acid sidechain carry a positive charge at pH 7?
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].property & POSITIVE_CHARGE;
}


/*------------------------------------------------------------*/
boolean
aa_negative_charge (int aa_int)
     /*
       Does the amino-acid sidechain carry a negative charge at pH 7?
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].property & NEGATIVE_CHARGE;
}


/*------------------------------------------------------------*/
boolean
aa_charged (int aa_int)
     /*
       Does the amino-acid sidechain carry a charge at pH 7?
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].property & (POSITIVE_CHARGE | NEGATIVE_CHARGE);
}


/*------------------------------------------------------------*/
boolean
aa_zn_binding (int aa_int)
     /*
       Is the amino-acid sidechain potentially Zn-binding?
     */
{
  /* pre */
  assert (aa_int >= 0);
  assert (aa_int < 20);

  return aa_nodes[aa_int].property & ZN_BINDING;
}


/*------------------------------------------------------------*/
int
aa_random (double rnd)
     /*
       Return an amino-acid ordinal for the random number.
     */
{
  int aa;

  /* pre */
  assert (rnd >= 0.0);
  assert (rnd <= 1.0);

  aa = (int) (20.0 * rnd);
  if (aa == 20) aa = 19;

  return aa;
}


/*------------------------------------------------------------*/
int
aa_random_abundance (double rnd)
     /*
       Return an amino-acid ordinal for the random number, such that
       the distribution follows the relative abundances in Swissprot.
     */
{
  int aa;

  /* pre */
  assert (rnd >= 0.0);
  assert (rnd <= 1.0);

  for (aa = 0; aa < 20; aa++) {
    rnd -= aa_nodes[aa].abundance;
    if (rnd <= 0.0) return aa;
  }
  return -1;
}


/*------------------------------------------------------------*/
int
aa_random_distribution (double rnd, double distribution[20])
     /*
       Return an amino-acid ordinal for the random number, controlled
       by the given distribution, which should sum to 1.0.
     */
{
  int aa;

  /* pre */
  assert (rnd >= 0.0);
  assert (rnd <= 1.0);

  for (aa = 0; aa < 20; aa++) {
    rnd -= distribution[aa];
    if (rnd <= 0.0) return aa;
  }
  return -1;
}
