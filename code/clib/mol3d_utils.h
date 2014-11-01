#ifndef MOL3D_UTILS_H
#define MOL3D_UTILS_H 1

#include <stdio.h>

#include <mol3d.h>
#include <mol3d_init.h>

void
mol3d_fprintf_count (FILE *file, mol3d *mol);

void
mol3d_fprintf_count_all (FILE *file, mol3d *first_mol);

void
mol3d_fprintf_residues (FILE *file, mol3d *mol);

at3d **
mol3d_atom_list (mol3d *mol);

at3d **
mol3d_atom_list_all (mol3d *first_mol);

res3d **
mol3d_residue_list (mol3d *mol);

res3d **
mol3d_residue_list_all (mol3d *first_mol);

int
mol3d_delete_altloc_atoms (mol3d *mol);

int
mol3d_delete_residue_type (mol3d *mol, char *type);

void
mol3d_extent (mol3d *mol, vector3 *low, vector3 *high);

double
mol3d_max_radius (mol3d *mol);

boolean
res3d_unique_name (mol3d *mol, res3d *res);

res3d *
res3d_find_aa_number (mol3d *mol, int number);

int
res3d_aa_number (mol3d *mol, res3d *res);

#endif
