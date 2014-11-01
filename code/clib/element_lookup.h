/* element_lookup.h

   Chemical element symbol lookup.

   clib v1.1

   Copyright (C) 1998 Per Kraulis
     2-Mar-1998  first attempts
     4-Mar-1998  written
*/

#ifndef ELEMENT_LOOKUP_H
#define ELEMENT_LOOKUP_H 1

#include <boolean.h>

extern const int element_max_number;

int element_number (const char *symbol);
int element_number_convert (const char *symbol);
int element_valid_number (int number);
char *element_symbol (int element_number);

#endif
