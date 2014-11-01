/* select.h

   MolScript v2.1.2

   Atom and residue selection definitions.

   Copyright (C) 1997-1998 Per Kraulis
     7-Dec-1996  first attempts
     2-Jan-1997  largely finished
*/

#include "coord.h"

typedef struct selection selection;

struct selection {
  int *flags;
  selection *next, *prev;
};

void push_atom_selection (void);
void push_residue_selection (void);
void pop_atom_selection (void);
void pop_residue_selection (void);

int count_atom_selections (void);
int count_residue_selections (void);

int select_atom_count (void);
int select_residue_count (void);

at3d **select_atom_list (int *atom_count);

void select_atom_not (void);
void select_atom_and (void);
void select_atom_or (void);

void select_residue_not (void);
void select_residue_and (void);
void select_residue_or (void);

void select_atom_id (const char *item);
void select_atom_res_id (const char *item);
void select_atom_occupancy (void);
void select_atom_b_factor (void);
void select_atom_in (void);
void select_atom_sphere (void);
void select_atom_close (void);
void select_atom_backbone (void);
void select_atom_peptide (void);
void select_atom_hydrogens (void);
void select_atom_element (const char *item);

void select_residue_molecule (const char *item);
void select_residue_model (void);
void select_residue_from_to (const char *item1, const char *item2);
void select_residue_id (const char *item);
void select_residue_type (const char *item);
void select_residue_chain (const char *item);
void select_residue_contains (void);
void select_residue_amino_acids (void);
void select_residue_waters (void);
void select_residue_nucleotides (void);
void select_residue_ligands (void);
void select_residue_segid (const char *item);

extern selection *current_atom_sel;
extern selection *current_residue_sel;
