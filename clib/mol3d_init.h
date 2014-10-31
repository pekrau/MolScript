#ifndef MOL3D_INIT_H
#define MOL3D_INIT_H 1

#include <mol3d.h>

void
mol3d_init (mol3d *mol, unsigned int flags);

void
mol3d_init_noblanks (mol3d *mol);

void
mol3d_init_colours (mol3d *mol);

void
mol3d_init_radii (mol3d *mol);

void
mol3d_init_aacodes (mol3d *mol);

void
mol3d_init_centrals (mol3d *mol, const char *atomname);

void
mol3d_init_centrals_protein (mol3d *mol);

void
mol3d_init_residue_ordinals (mol3d *mol, int start);

void
mol3d_init_residue_ordinals_protein (mol3d *mol);

void
mol3d_init_atom_ordinals (mol3d *mol, int start);

void
mol3d_init_elements (mol3d *mol);

#endif
