#ifndef STR_UTILS_H
#define STR_UTILS_H 1

#include <boolean.h>

boolean
str_eq (const char *str1, const char *str2);

boolean
str_eqn (const char *str1, const char *str2, const int n);

boolean
str_eq_min  (const char *str1, const char *str2);

void 
str_fill_blanks (char *str, int const length);

void
str_remove_blanks (char *str);

void
str_remove_nulls (char *str, int const length);

void
str_left_adjust (char *str);

void
str_right_adjust (char *str);

void
str_exchange (char *str, char old, char new);

void
str_upcase (char *str);

void
str_lowcase (char *str);

char *
str_alloc (const int length);

char *
str_clone (const char *str);

char *
str_subclone (const char *str, const int first, const int last);

int
str_hashcode (const char *str, const int range);

#endif
