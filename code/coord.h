/* coord.h

   MolScript v2.1.2

   Coordinate data handling.

   Copyright (C) 1997-1998 Per Kraulis
     4-Dec-1996  first attempts
     3-Jan-1997  fairly finished
    30-Jan-1997  use clib mol3d
*/

#ifndef COORD_H
#define COORD_H 1

#include "clib/mol3d_io.h"
#include "clib/mol3d_init.h"
#include "clib/mol3d_chain.h"


#define PEPTIDE_CHAIN_ATOMNAME "CA"
#define PEPTIDE_DISTANCE 4.2
#define NUCLEOTIDE_CHAIN_ATOMNAME "P"
#define NUCLEOTIDE_DISTANCE 10.0

extern mol3d *first_molecule;
extern int total_atoms;
extern int total_residues;

void store_molname (char *name);
void read_coordinate_file (char *filename);
void init_molecule (mol3d *mol);
void update_totals (void);

void copy_molecule (char *name);
void delete_molecule (char *name);
void delete_all_molecules (void);

mol3d_chain *get_peptide_chains (void);
mol3d_chain *get_nucleotide_chains (void);

void position (void);

#endif
