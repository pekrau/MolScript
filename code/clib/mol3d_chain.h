#ifndef MOL3D_CHAIN_H
#define MOL3D_CHAIN_H 1

#include <mol3d.h>
#include <mol3d_init.h>

typedef struct s_mol3d_chain mol3d_chain;

struct s_mol3d_chain {
  int length;
  res3d **residues;
  at3d **atoms;
  mol3d_chain *next;
};

mol3d_chain *
mol3d_chain_find (mol3d *first_mol, char *atomname,
		  double cutoff, int *res_sel);

void
mol3d_chain_delete (mol3d_chain *first_ch);

#endif
