#ifndef INDENT_H
#define INDENT_H 1

#include <stdio.h>

#include <boolean.h>

#define INDENT_FPRINTF indent_buflen+=fprintf

extern FILE *indent_file;
extern boolean indent_flag;
extern int indent_level;
extern int indent_increment;
extern boolean indent_needs_blank;
extern int indent_buflen;
extern int indent_max_buflen;

void
indent_initialize (FILE *file);

boolean
indent_valid_state (void);

void
indent_putchar (char c);

void
indent_string (char *str);

void
indent_blank (void);

void
indent_newline (void);

void
indent_newline_conditional (void);

void
indent_check_buflen (int additional);

void
indent_set_indent (boolean on);

void
indent_set_level (int new);

void
indent_set_max_buflen (int new);

void
indent_increment_level (boolean incr);

#endif
