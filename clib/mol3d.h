#ifndef MOL3D_H
#define MOL3D_H 1

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

mol3d *
mol3d_create (void);

res3d *
res3d_create (void);

at3d *
at3d_create (void);

res3d *
res3d_clone (res3d *res);

at3d *
at3d_clone (at3d *at);

void
mol3d_delete (mol3d *mol);

void
mol3d_delete_all (mol3d *first_mol);

void
res3d_delete (res3d *res);

void
at3d_delete (at3d *at);

void
mol3d_set_name (mol3d *mol, const char *name);

mol3d *
mol3d_append (mol3d *first_mol, mol3d *new);

res3d *
res3d_add (res3d *res, res3d *new);

at3d *
at3d_add (at3d *at, at3d *new);

res3d *
mol3d_append_residue (mol3d *mol, res3d *res);

at3d *
res3d_append_atom (res3d *res, at3d *at);

named_data *
mol3d_create_named_data (mol3d *mol, char *name, boolean ncopy,
			 void *data, boolean dcopy);

named_data *
res3d_create_named_data (res3d *res, char *name, boolean ncopy,
			 void *data, boolean dcopy);

named_data *
at3d_create_named_data (at3d *at, char *name, boolean ncopy,
			void *data, boolean dcopy);

boolean
mol3d_remove_molecule (mol3d *mol1, mol3d *mol2);

boolean
mol3d_remove_residue (mol3d *mol, res3d *res);

boolean
res3d_remove_atom (res3d *res, at3d *at);

int
mol3d_count (mol3d *first_mol);

int
mol3d_count_residues (mol3d *mol);

int
mol3d_count_residues_all (mol3d *first_mol);

int
mol3d_count_atoms (mol3d *mol);

int
mol3d_count_atoms_all (mol3d *first_mol);

int
res3d_count_atoms (res3d *res);

mol3d *
mol3d_lookup (mol3d *first_mol, const char *name);

res3d *
res3d_lookup (mol3d *mol, const char *name);

at3d *
at3d_lookup (res3d *res, const char *name);

res3d *
res3d_find_ordinal (mol3d *mol, int ordinal);

at3d *
at3d_find_ordinal (mol3d *mol, int ordinal);

void
mol3d_do_residues (mol3d *mol, void (*proc) (res3d *res));

void
mol3d_do_residues_all (mol3d *first_mol, void (*proc) (res3d *res));

void
mol3d_do_atoms (mol3d *mol, void (*proc) (at3d *at));

void
mol3d_do_atoms_all (mol3d *first_mol, void (*proc) (at3d *at));

#endif
