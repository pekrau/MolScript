#ifndef MOL3D_IO_H
#define MOL3D_IO_H 1

#include <stdio.h>

#include <mol3d.h>

enum mol3d_file_types { MOL3D_UNKNOWN_FILE, MOL3D_PDB_FILE, MOL3D_MSA_FILE,
			MOL3D_DG_FILE, MOL3D_RD_FILE, MOL3D_CDS_FILE,
			MOL3D_WAH_FILE };

int
mol3d_file_type (char *filename);

boolean
mol3d_file_is_pdb (FILE *file);

mol3d *
mol3d_read_pdb_file (FILE *file);

mol3d *
mol3d_read_pdb_filename (char *filename);

boolean
mol3d_is_pdb_code (char *code);

mol3d *
mol3d_read_pdb_code (char *code);

boolean
mol3d_write_pdb_file (FILE *file, mol3d *first_mol);

boolean
mol3d_write_pdb_header (FILE *file, mol3d *mol);

int
mol3d_write_pdb_file_mol (FILE *file, mol3d *mol);

boolean
mol3d_write_pdb_filename (char *filename, mol3d *first_mol);

#endif
