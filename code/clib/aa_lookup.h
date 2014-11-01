#ifndef AA_LOOKUP_H
#define AA_LOOKUP_H 1

#include <boolean.h>

int
aa_ordinal (char *c3);

char
aa_3to1 (char *c3);

char *
aa_1to3 (char c1);

char
aa_all3to1 (char *c3);

int
aa_int (char c1);

char
aa_code (int aa_int);

boolean
is_amino_acid_type (char *c3);

boolean
is_water_type (char *c3);

boolean
is_nucleic_acid_type (char *c3);

double
aa_abundance (int aa_int);

double
aa_fauchere_pliska (int aa_int);

double
aa_fauchere_pliska_normalized (int aa_int);

double
aa_kyte_doolittle (int aa_int);

double
aa_kyte_doolittle_normalized (int aa_int);

boolean
aa_conformation_special (int aa_int);

boolean
aa_ring (int aa_int);

boolean
aa_hbond_forming (int aa_int);

boolean
aa_hbond_donor (int aa_int);

boolean
aa_hbond_acceptor (int aa_int);

boolean
aa_polar (int aa_int);

boolean
aa_aromatic (int aa_int);

boolean
aa_positive_charge (int aa_int);

boolean
aa_negative_charge (int aa_int);

boolean
aa_charged (int aa_int);

boolean
aa_zn_binding (int aa_int);

int
aa_random (double rnd);

int
aa_random_abundance (double rnd);

int
aa_random_distribution (double rnd, double distribution[20]);

#endif
