#ifndef MOL3D_SECSTRUC_H
#define MOL3D_SECSTRUC_H 1

#include <mol3d.h>

void
mol3d_secstruc_initialize (mol3d *mol);

void
mol3d_secstruc_ca_geom (mol3d *mol);

boolean
mol3d_secstruc_hbonds (mol3d *mol);

#endif
