/* lex.h

   MolScript v2.1.2

   Input lexer.

   Copyright (C) 1997-1998 Per Kraulis
    14-Dec-1996  first attempts
     1-Aug-1997  cleanup procedure
    15-Jan-1998  added yytext stack
*/

#ifndef LEX_H
#define LEX_H 1

#include <stdio.h>

extern char yytext[];
extern int yylen;

void lex_init (void);
void lex_info (void);
void lex_cleanup (void);
void lex_set_input_file (const char *filename);
FILE *lex_input_file (void);
void lex_define_macro (char *name);
void lex_yytext_push (void);
void lex_yytext_pop (void);
int lex_yytext_depth (void);
char *lex_yytext_str (void);

int yylex (void);

#endif
