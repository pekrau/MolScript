/* mol3d_io

   Molecule coordinate data file input/output.

   clib v1.1

   Copyright (C) 1997-1998 Per Kraulis
     5-Apr-1997  optimized PDB file read procedure
    28-Jan-1998  rewrite PDB read, added at3d ordinal and related routines
     4-Mar-1998  first working PDB write routine
     4-May-1998  broken out of mol3d, modified write procedures
     4-Jun-1998  set secondary structure directly if given
    25-Nov-1998  fixed bug in mol3d_read_pdb_file
*/

#include "mol3d_io.h"

/* public ====================
#include <stdio.h>

#include <mol3d.h>

enum mol3d_file_types { MOL3D_UNKNOWN_FILE, MOL3D_PDB_FILE, MOL3D_MSA_FILE,
			MOL3D_DG_FILE, MOL3D_RD_FILE, MOL3D_CDS_FILE,
			MOL3D_WAH_FILE };
==================== public */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <str_utils.h>
#include <io_utils.h>
#include <key_value.h>
#include <element_lookup.h>
#include <aa_lookup.h>
#include <mol3d_init.h>


/*============================================================*/
static char HEADER[] = "HEADER";
static char CLASSIFICATION[] = "CLASSIFICATION";
static char TITLE[] = "TITLE";
static char CAVEAT[] = "CAVEAT";
static char COMPND[] = "COMPND";
static char SOURCE[] = "SOURCE";
static char KEYWDS[] = "KEYWDS";
static char EXPDTA[] = "EXPDTA";
static char AUTHOR[] = "AUTHOR";
static char JRNL[] = "JRNL";
static char HELIX[] = "HELIX";
static char SHEET[] = "SHEET";
static char TURN[] = "TURN";
static char CRYST1[] = "CRYST1";
static char REMARK[] = "REMARK";
static char ATOM[] = "ATOM";
static char HETATM[] = "HETATM";
static char MODEL[] = "MODEL";
static char ENDMDL[] = "ENDMDL";
static char END[] = "END";
static char FORMAT_VERSION[] = "FORMAT_VERSION";


/*------------------------------------------------------------*/
int
mol3d_file_type (char *filename)
     /*
       Return the coordinate file type code based on the extension
       in the file name.
     */
{
  int result = MOL3D_UNKNOWN_FILE;
  int len;
  char *suffix;

  /* pre */
  assert (filename);

  len = strlen (filename);
  if (len > 2) {
    suffix = filename + len - 3;
    if (str_eq (suffix, ".dg") || str_eq (suffix, ".DG")) {
      result = MOL3D_DG_FILE;
    } else if (str_eq (suffix, ".rd") || str_eq (suffix, ".RD")) {
      result = MOL3D_RD_FILE;
    } else if (len > 3) {
      suffix = filename + len - 4;
      if (str_eq (suffix, ".pdb") || str_eq (suffix, ".PDB") ||
	  str_eq (suffix, ".ent") || str_eq (suffix, ".ENT")) {
	result = MOL3D_PDB_FILE;
      } else if (str_eq (suffix, ".msa") || str_eq (suffix, ".MSA")) {
	result = MOL3D_MSA_FILE;
      } else if (str_eq (suffix, ".cds") || str_eq (suffix, ".CDS")) {
	result = MOL3D_CDS_FILE;
      } else if (str_eq (suffix, ".wah") || str_eq (suffix, ".WAH")) {
	result = MOL3D_WAH_FILE;
      }
    }
  }

  return result;
}


/*------------------------------------------------------------*/
boolean
mol3d_file_is_pdb (FILE *file)
     /*
       Test whether the opened file is a PDB file; the first record is
       checked to see if it is a HEADER, REMARK, ATOM or HETATM record.
     */
{
  fpos_t orig_pos;
  char line[7];

  /* pre */
  assert (file);

  if (fgetpos (file, &orig_pos)) return FALSE;
  if (fgets (line, 6, file) == NULL) return FALSE;
  if (fsetpos (file, &orig_pos)) return FALSE;

  return (str_eq_min (line, HEADER) ||
	  str_eq_min (line, REMARK) ||
	  str_eq_min (line, ATOM) ||
	  str_eq_min (line, HETATM));
}


/*------------------------------------------------------------*/
static void
mol3d_append_pdb_named_data (mol3d *mol, char *name,
			     char *record, int len, int first, int last)
{
  dynstring *ds;

  assert (mol);
  assert (name);
  assert (*name);
  assert (record);
  assert (*record);
  assert (first >= 0);
  assert (first <= last);

  ds = (dynstring *) nd_search_data (mol->data, name);
  if (ds) {
    if (record[first] != ' ') ds_add (ds, ' ');
    ds_subcat (ds, record, first, (len <= last) ? (len - 1) : last);
  } else {
    ds = ds_subcreate (record, first, (len <= last) ? (len - 1) : last);
    mol3d_create_named_data (mol, name, FALSE, ds, TRUE);
  }
  ds_right_adjust (ds);
}


/*------------------------------------------------------------*/
mol3d *
mol3d_read_pdb_file (FILE *file)
     /*
       Read the coordinate set contained in the opened file and return
       the molecule(s). NULL is returned if there was an error.
     */
{
#define PDB_RECORD_LENGTH 80
  mol3d *first_mol, *mol;
  char record [PDB_RECORD_LENGTH + 4]; /* allow for files with CR-LF lines */
  res3d *res;
  at3d *at, *new_at;
  char *str;
  int length;
  dynstring *ds;
  char resname [RES3D_NAME_LENGTH + 1];
  char prev_resname [RES3D_NAME_LENGTH + 1];
  char restype [RES3D_TYPE_LENGTH + 1];
  char prev_restype [RES3D_TYPE_LENGTH + 1];
  boolean new_format = FALSE;
  key_value *first_ss = NULL, *ss;

  /* pre */
  assert (file);

  str_fill_blanks (resname, RES3D_NAME_LENGTH);
  str_fill_blanks (prev_resname, RES3D_NAME_LENGTH);
  str_fill_blanks (restype, RES3D_TYPE_LENGTH);
  str_fill_blanks (prev_restype, RES3D_TYPE_LENGTH);

  first_mol = mol3d_create();
  mol = first_mol;
				/* header records */
  str = fgets (record, PDB_RECORD_LENGTH + 3, file);
  if (str == NULL) goto error;

  while (str) {

    if ((str_eq_min (ATOM, record)) || /* skip to atom-reading loop */
	(str_eq_min (HETATM, record))) break;

    length = strlen (record);
    if ((length > 0) &&		/* get rid of newline */
	(record[length-1] == '\n')) record[--length] = '\0';

    if (str_eq_min (HEADER, record)) {
      if (length < 66) goto error;

      ds = ds_subcreate (record, 62, 65);
      mol3d_set_name (mol, ds->string);
      ds_subset (ds, record, 10, 49);
      ds_right_adjust (ds);
      mol3d_create_named_data (mol, CLASSIFICATION, FALSE, ds, TRUE);

    } else if (str_eq_min (TITLE, record)) {
      mol3d_append_pdb_named_data (mol, TITLE, record, length, 10, 69);

    } else if (str_eq_min (CAVEAT, record)) {
      mol3d_append_pdb_named_data (mol, CAVEAT, record, length, 10, 69);

    } else if (str_eq_min (COMPND, record)) {
      mol3d_append_pdb_named_data (mol, COMPND, record, length, 10, 69);

    } else if (str_eq_min (SOURCE, record)) {
      mol3d_append_pdb_named_data (mol, SOURCE, record, length, 10, 69);

    } else if (str_eq_min (KEYWDS, record)) {
      mol3d_append_pdb_named_data (mol, KEYWDS, record, length, 10, 69);

    } else if (str_eq_min (EXPDTA, record)) {
      mol3d_append_pdb_named_data (mol, EXPDTA, record, length, 10, 69);

    } else if (str_eq_min (AUTHOR, record)) {
      mol3d_append_pdb_named_data (mol, AUTHOR, record, length, 10, 69);

    } else if (str_eq_min (JRNL, record)) {
      ds = ds_subcreate (record, 12, (length < 70) ? (length - 1) : 69); 
      ds_right_adjust (ds);
      mol3d_create_named_data (mol, JRNL, FALSE, ds, TRUE);

    } else if (str_eq_min (REMARK, record)) {

      if (str_eq_min (record + 7, "  4") &&
	  (length >= 43) &&
	  str_eq_min (record + 16, "COMPLIES WITH FORMAT V. ")) {
	ds = ds_subcreate (record, 40, 42);
	mol3d_create_named_data (mol, FORMAT_VERSION, FALSE, ds, TRUE);
	new_format = TRUE;
      }

    } else if (str_eq_min (HELIX, record)) {
      ss = kv_create (HELIX, NULL);
      ds_subcat (ss->value, record, 15, 17);
      ds_subcat (ss->value, record, 19, 19);
      ds_subcat (ss->value, record, 21, 25);
      ds_subcat (ss->value, record, 27, 29);
      ds_subcat (ss->value, record, 31, 31);
      ds_subcat (ss->value, record, 33, 37);
      if (first_ss) kv_append (first_ss, ss); else first_ss = ss;

    } else if (str_eq_min (SHEET, record)) {
      ss = kv_create (SHEET, NULL);
      ds_subcat (ss->value, record, 17, 19);
      ds_subcat (ss->value, record, 21, 26);
      ds_subcat (ss->value, record, 28, 30);
      ds_subcat (ss->value, record, 32, 37);
      if (first_ss) kv_append (first_ss, ss); else first_ss = ss;

    } else if (str_eq_min (TURN, record)) {
      ss = kv_create (TURN, NULL);
      ds_subcat (ss->value, record, 15, 17);
      ds_subcat (ss->value, record, 19, 24);
      ds_subcat (ss->value, record, 26, 28);
      ds_subcat (ss->value, record, 30, 35);
      if (first_ss) kv_append (first_ss, ss); else first_ss = ss;

    } else if (str_eq_min (MODEL, record)) {
      sscanf (record, "%*10c%i", &(mol->model));

    } else if (str_eq_min (CRYST1, record)) {
      ds = ds_subcreate (record, 12, (length < 70) ? (length - 1) : 69); 
      ds_right_adjust (ds);
      mol3d_create_named_data (mol, CRYST1, FALSE, ds, TRUE);
    }

    str = fgets (record, PDB_RECORD_LENGTH + 3, file);
    if (str == NULL) goto error;
  }

  res = NULL;
  at = NULL;
  assert (mol);

  while (str) {			/* atom records */
    length = strlen (record);

    if ((str_eq_min (ATOM, record)) ||
	(str_eq_min (HETATM, record))) {

      new_at = at3d_create();
      record[66] = '\0';	/* this is faster than using sscanf */
      new_at->bfactor = atof (record + 60);
      record[60] = '\0';
      new_at->occupancy = atof (record + 54);
      record[54] = '\0';
      new_at->xyz.z = atof (record + 46);
      record[46] = '\0';
      new_at->xyz.y = atof (record + 38);
      record[38] = '\0';
      new_at->xyz.x = atof (record + 30);
      strncpy (resname, record + 21, 6);
      strncpy (restype, record + 17, 3);
      strncpy (new_at->name, record + 12, 4);
      new_at->name[4] = '\0';
      new_at->altloc = record[16];

      if (length >= 78) {
	new_at->element = element_number_convert (record + 76);
	if ((length >= 80) &&
	    (record[78] != ' ') &&
	    ((record[79] == '+') || (record[79] == '-'))) {
	  new_at->charge = (double) atoi (record + 78);
	  if (record[79] == '-') new_at->charge = - new_at->charge;
	}
      }

      if (! str_eq (resname, prev_resname) ||
	  ! str_eq (restype, prev_restype)) {
	res3d *new_res = res3d_create();
	strcpy (new_res->name, resname);
	strcpy (new_res->type, restype);
	new_res->chain = resname[0];
	new_res->heterogen = str_eq_min (HETATM, record);
	if (length >= 76) strncpy (new_res->segid, record + 72, 4);

	strcpy (prev_resname, resname);
	strcpy (prev_restype, restype);

	if (res) {
	  res = res3d_add (res, new_res);
	} else {
	  res = mol3d_append_residue (mol, new_res);
	}
	at = res3d_append_atom (res, new_at);

      } else {
	at = at3d_add (at, new_at);
      }

    } else if (str_eq_min (MODEL, record)) {
      sscanf (record, "%*10c%i", &(mol->model));

    } else if (str_eq_min (ENDMDL, record)) {

      mol3d *new_mol = mol3d_create();
      if (mol->name) new_mol->name = str_clone (mol->name);
      mol3d_append (mol, new_mol);
      mol = new_mol;

      str_fill_blanks (resname, RES3D_NAME_LENGTH);
      str_fill_blanks (prev_resname, RES3D_NAME_LENGTH);
      str_fill_blanks (restype, RES3D_TYPE_LENGTH);
      str_fill_blanks (prev_restype, RES3D_TYPE_LENGTH);
      res = NULL;
      at = NULL;

    } else if (str_eq_min (END, record)) {
      break;
    }

    str = fgets (record, PDB_RECORD_LENGTH + 3, file);
    if (str == NULL) break;
  }

  if (first_mol->first == NULL) goto error;

  for (mol = first_mol; mol->next; mol = mol->next) { /* remove mol's */
    if (mol3d_count_atoms (mol->next) == 0) {         /* having no atoms */
      mol3d *mol2 = mol->next;
      mol->next = NULL;
      mol3d_delete (mol2);
      break;
    }
  }

  if (new_format) {		/* new format files give elements; set flag */
    for (mol = first_mol; mol; mol = mol->next) {
      mol->init |= MOL3D_INIT_ELEMENTS;
    }
  }

  if (first_ss) {		/* set the secondary structure, if given */
    char secstruc;

    for (mol = first_mol; mol; mol = mol->next) {
      for (res = mol->first; res; res = res->next) {
	if (is_amino_acid_type (res->type)) {
	  res->secstruc = ' ';
	} else {
	  res->secstruc = '-';
	}
      }
    }

    for (mol = first_mol; mol; mol = mol->next) {
      for (ss = first_ss; ss; ss = ss->next) {
        for (res = mol->first; res; res = res->next) {
          if (str_eqn (ss->value->string + 3, res->name, 6) &&
              str_eqn (ss->value->string, res->type, 3)) {
            if (str_eq (ss->key->string, HELIX)) {
              res->secstruc = 'h';
            } else if (str_eq (ss->key->string, SHEET)) {
              res->secstruc = 'e';
            } else if (str_eq (ss->key->string, TURN)) {
              res->secstruc = 't';
            }
            secstruc = toupper (res->secstruc);
            for (res = res->next; res; res = res->next) {
              res->secstruc = secstruc;
              if (str_eqn (ss->value->string + 12, res->name, 6) &&
                  str_eqn (ss->value->string + 9, res->type, 3)) break;
            }
            break;
          }
        }
      }
      mol->init |= MOL3D_INIT_SECSTRUC;
    }

    kv_delete (first_ss);
  }

  return first_mol;

 error:
  mol3d_delete (first_mol);
  return NULL;
}


/*------------------------------------------------------------*/
mol3d *
mol3d_read_pdb_filename (char *filename)
     /*
       Read the coordinate set in the file with the given name.
       NULL is returned if the file could not be opened or read.
     */
{
  FILE *file;
  mol3d *mol;

  /* pre */
  assert (filename);
  assert (*filename);

  file = fopen (filename, "r");
  if (file == NULL) return NULL;

  mol = mol3d_read_pdb_file (file);
  fclose (file);

  return mol;
}


/*------------------------------------------------------------*/
boolean
mol3d_is_pdb_code (char *code)
     /*
       Is the code a valid PDB coordinate set code?
     */
{
  if (code == NULL) return FALSE;
  if (strlen (code) != 4) return FALSE;
  if (! isdigit (*code)) return FALSE;
  return TRUE;
}


/*------------------------------------------------------------*/
mol3d *
mol3d_read_pdb_code (char *code)
     /*
       Read the PDB coordinate set given by the code, from the PDB
       distribution located in the directory given by the environment
       variable MOL3D_PDB_DIR. Three different directory and file name
       conventions are tested consecutively. These are:
       1. PDB standard hierarchical organisation and file name:
          for code 1XYZ the file is $MOL3D_PDB_DIR/distr/xy/pdb1xyz.ent
       2. PDB flat directory, standard file name:
          for code 1XYZ the file is $MOL3D_PDB_DIR/pdb1xyz.ent
       3. PDB flat directory, simplified file name:
          for code 1XYZ the file is $MOL3D_PDB_DIR/1xyz.pdb
       The given PDB code is changed to lower case characters, and the
       other strings used to construct the file name are all in lower case,
       except the environment variable value which is used as is.
       Return NULL if the environment value is undefined, or the file
       could not be opened.
     */
{
  char *code_copy, *pdb_dir;
  dynstring *ds;
  mol3d *mol;

  /* pre */
  assert (mol3d_is_pdb_code (code));

  code_copy = str_clone (code);
  str_lowcase (code_copy);

  pdb_dir = getenv ("MOL3D_PDB_DIR");
  if (pdb_dir == NULL) return NULL;

  ds = ds_create (pdb_dir);	/* attempt 1; standard hierarchy and name */
  if (pdb_dir[strlen (pdb_dir) - 1] != '/') ds_add (ds, '/');
  ds_cat (ds, "distr/");
  ds_subcat (ds, code_copy, 1, 2);
  ds_cat (ds, "/pdb");
  ds_cat (ds, code_copy);
  ds_cat (ds, ".ent");
  mol = mol3d_read_pdb_filename (ds->string);
  if (mol) goto finish;

  ds_set (ds, pdb_dir);		/* attempt2: flat dir, standard name */
  if (pdb_dir[strlen (pdb_dir) - 1] != '/') ds_add (ds, '/');
  ds_cat (ds, "pdb");
  ds_cat (ds, code_copy);
  ds_cat (ds, ".ent");
  mol = mol3d_read_pdb_filename (ds->string);
  if (mol) goto finish;

  ds_set (ds, pdb_dir);		/* attempt 3: flat dir, simplified name */
  if (pdb_dir[strlen (pdb_dir) - 1] != '/') ds_add (ds, '/');
  ds_cat (ds, code_copy);
  ds_cat (ds, ".pdb");
  mol = mol3d_read_pdb_filename (ds->string);

finish:
  ds_delete (ds);
  free (code_copy);

  return mol;
}


/*------------------------------------------------------------*/
boolean
mol3d_write_pdb_file (FILE *file, mol3d *first_mol)
     /*
       Write the molecules in the linked list including the given molecule
       in PDB format to the opened file.
     */
{
  mol3d *mol;

  /* pre */
  assert (file);
  assert (first_mol);

  if (! mol3d_write_pdb_header (file, first_mol)) return FALSE;
  for (mol = first_mol; mol; mol = mol->next) {
    if (! mol3d_write_pdb_file_mol (file, mol)) return FALSE;
  }
  fprintf (file, "%s\n", END);

  return TRUE;
}


/*------------------------------------------------------------*/
boolean
mol3d_write_pdb_header (FILE *file, mol3d *mol)
     /*
       Write the PDB format header to the opened file.
     */
{
  named_data *nd;
  char code[5];

  /* pre */
  assert (file);
  assert (mol);

  if (mol->name) {
    strncpy (code, mol->name, 4);
  } else {
    code[0] = '\0';
  }

  if (! io_fprint_str_length (file, HEADER, 11)) return FALSE;
  nd = nd_search (mol->data, CLASSIFICATION);
  if (nd) {
    io_fprint_str_length (file, (char *) (((dynstring *) nd->data)->string),
			 50 - 11 + 1);
  } else {
    io_fprint_blanks (file, 50 - 11 + 1);
  }
  io_fprint_blanks (file, 63 - 51 + 1);
  fprintf (file, "%4.4s", *code ? code : "0XXX");
  fputc ('\n', file);

  if (nd_search (mol->data, FORMAT_VERSION)) {
    fprintf (file,
	     "REMARK   4 %4.4s COMPLIES WITH FORMAT V. 2.0, 15-APR-1996\n",
	     code);
  }

  return TRUE;
}


/*------------------------------------------------------------*/
int
mol3d_write_pdb_file_mol (FILE *file, mol3d *mol)
{
  res3d *res;
  at3d *at;
  char resnumber[RES3D_NAME_LENGTH + 1];
  char chainid, insertion;
  int length, charge;
  boolean new_format;
  char *el_sym, symbol[3];

  assert (file);
  assert (mol);

  if (mol->model != 0) {
    io_fprint_str_length (file, MODEL, 11);
    fprintf (file, "%4i\n", mol->model);
  }

  new_format = mol->init & MOL3D_INIT_ELEMENTS;

  for (res = mol->first; res; res = res->next) {

    strcpy (resnumber, res->name);
    length = strlen (resnumber);
    if (isalpha (resnumber[0])) {
      chainid = resnumber[0];
    } else {
      chainid = ' ';
    }
    length--;
    if (isalpha (resnumber[length])) {
      insertion = resnumber[length];
      resnumber[length] = '\0';
    } else {
      insertion = ' ';
    }

    for (at = res->first; at; at = at->next) {
      fprintf (file,
	       "%-6s%5i %-4s%c%3s %c%4s%c   %8.3f%8.3f%8.3f%6.2f%6.2f      %4.4s",
	       res->heterogen ? HETATM : ATOM, at->ordinal % 100000, at->name,
	       at->altloc, res->type, chainid,
	       (chainid != ' ') ? resnumber + 1 : resnumber, insertion,
	       at->xyz.x, at->xyz.y, at->xyz.z, at->occupancy, at->bfactor,
	       res->segid);
      if (new_format) {
	el_sym = element_symbol (at->element);
	if (el_sym) {
	  strcpy (symbol, el_sym);
	  str_upcase (symbol);
	} else {
	  symbol[0] = ' ';
	  symbol[1] = ' ';
	  symbol[2] = '\0';
	}
	fprintf (file, "%2.2s", symbol);
      }
      fputc ('\n', file);
    }
  }

  if (mol->model != 0) fprintf (file, "%s\n", ENDMDL);

  return (ferror (file) == 0);
}


/*------------------------------------------------------------*/
boolean
mol3d_write_pdb_filename (char *filename, mol3d *first_mol)
     /*
       Write the molecules in the linked list including the given molecule
       to a file with the given name. Return TRUE if successful.
     */
{
  FILE *file;
  boolean success;

  /* pre */
  assert (filename);
  assert (*filename);
  assert (first_mol);

  file = fopen (filename, "w");
  if (file == NULL) return FALSE;

  success = mol3d_write_pdb_file (file, first_mol);
  fclose (file);

  return success;
}
