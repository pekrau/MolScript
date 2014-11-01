#ifndef DYNSTRING_H
#define DYNSTRING_H 1

#include <boolean.h>

typedef struct {
  char *string;
  int capacity;
  int length;
} dynstring;

boolean
ds_valid_state (const dynstring *ds);

dynstring *
ds_create (const char *str);

dynstring *
ds_subcreate (const char *str, int first, int last);

dynstring *
ds_clone (dynstring *ds);

dynstring *
ds_subclone (dynstring *ds, int first, int last);

dynstring *
ds_allocate (const int capacity);

void
ds_reallocate (dynstring *ds, const int capacity);

void
ds_delete (dynstring *ds);

void
ds_set (dynstring *ds, const char *str);

void
ds_subset (dynstring *ds, const char *str, int first, int last);

void
ds_cat (dynstring *ds, const char *str);

void
ds_subcat (dynstring *ds, const char *str, int first, int last);

void
ds_add (dynstring *ds, const char c);

void
ds_cat_int (dynstring *ds, const int i);

void
ds_append (dynstring *ds1, const dynstring *ds2);

void
ds_replace (dynstring *ds, const int pos, const int len, const char *str);

void
ds_fill_blanks (dynstring *ds, const int length);

void
ds_remove_blanks (dynstring *ds);

void
ds_pad_blanks (dynstring *ds, int length);

void
ds_truncate (dynstring *ds, int length);

void
ds_left_adjust (dynstring *ds);

void
ds_right_adjust (dynstring *ds);

void
ds_reset (dynstring *ds);

void
ds_to_upper (dynstring *ds);

void
ds_to_lower (dynstring *ds);

#endif
