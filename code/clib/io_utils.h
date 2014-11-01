#ifndef IO_UTILS_H
#define IO_UTILS_H 1

#include <stdio.h>

#include <boolean.h>
#include <dynstring.h>

boolean
io_fprint_str (FILE *file, const char *str);

boolean
io_fprint_str_length (FILE *file, const char *str, int length);

boolean
io_fprint_blanks (FILE *file, int count);

boolean
io_fget_ds (FILE *file, dynstring *ds);

#endif
