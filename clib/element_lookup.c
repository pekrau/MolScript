/* element_lookup.c

   Chemical element symbol lookup.

   clib v1.1

   Copyright (C) 1998 Per Kraulis
     2-Mar-1998  first attempts
     4-Mar-1998  written
*/

#include "element_lookup.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/*============================================================*/
typedef struct {
  char *symbol;
  int number;
} element_node;

static element_node element_nodes[] = {
  {"H", 1},
  {"He", 2},
  {"Li", 3},
  {"Be", 4},
  {"B", 5},
  {"C", 6},
  {"N", 7},
  {"O", 8},
  {"F", 9},
  {"Ne", 10},
  {"Na", 11},
  {"Mg", 12},
  {"Al", 13},
  {"Si", 14},
  {"P", 15},
  {"S", 16},
  {"Cl", 17},
  {"Ar", 18},
  {"K", 19},
  {"Ca", 20},
  {"Sc", 21},
  {"Ti", 22},
  {"V", 23},
  {"Cr", 24},
  {"Mn", 25},
  {"Fe", 26},
  {"Co", 27},
  {"Ni", 28},
  {"Cu", 29},
  {"Zn", 30},
  {"Ga", 31},
  {"Ge", 32},
  {"As", 33},
  {"Se", 34},
  {"Br", 35},
  {"Kr", 36},
  {"Rb", 37},
  {"Sr", 38},
  {"Y", 39},
  {"Zr", 40},
  {"Nb", 41},
  {"Mo", 42},
  {"Tc", 43},
  {"Ru", 44},
  {"Rh", 45},
  {"Pd", 46},
  {"Ag", 47},
  {"Cd", 48},
  {"In", 49},
  {"Sn", 50},
  {"Sb", 51},
  {"Te", 52},
  {"I", 53},
  {"Xe", 54},
  {"Cs", 55},
  {"Ba", 56},
  {"La", 57},
  {"Ce", 58},
  {"Pr", 59},
  {"Nd", 60},
  {"Pm", 61},
  {"Sm", 62},
  {"Eu", 63},
  {"Gd", 64},
  {"Tb", 65},
  {"Dy", 66},
  {"Ho", 67},
  {"Er", 68},
  {"Tm", 69},
  {"Yb", 70},
  {"Lu", 71},
  {"Hf", 72},
  {"Ta", 73},
  {"W", 74},
  {"Re", 75},
  {"Os", 76},
  {"Ir", 77},
  {"Pt", 78},
  {"Au", 79},
  {"Hg", 80},
  {"Tl", 81},
  {"Pb", 82},
  {"Bi", 83},
  {"Po", 84},
  {"At", 85},
  {"Rn", 86},
  {"Fr", 87},
  {"Ra", 88},
  {"Ac", 89},
  {"Th", 90},
  {"Pa", 91},
  {"U", 92},
  {"Np", 93},
  {"Pu", 94},
  {"Am", 95},
  {"Cm", 96},
  {"Bk", 97},
  {"Cf", 98},
  {"Es", 99},
  {"Fm", 100},
  {"Md", 101},
  {"No", 102},
  {"Lr", 103},
  {"Db", 104},
  {"Jl", 105},
  {"Rf", 106},
  {"Bh", 107},
  {"Hn", 108},
  {"Mt", 109},
  {"Xa", 110},
  {"Xb", 111}
};

const int element_max_number = 111;


/*============================================================*/
static element_node *sorted_element_nodes = NULL;


/*------------------------------------------------------------*/
static int
qsort_compare (const void *es1, const void *es2)
{
  return strcmp (((element_node *) es1)->symbol,
		 ((element_node *) es2)->symbol);
}


/*------------------------------------------------------------*/
static void
initialize (void)
{
  sorted_element_nodes = malloc (element_max_number * sizeof (element_node));
  memcpy (sorted_element_nodes, element_nodes,
	  element_max_number * sizeof (element_node));
  qsort (sorted_element_nodes, element_max_number,
	 sizeof (element_node), qsort_compare);
}


/*------------------------------------------------------------*/
static int
bsearch_compare (const void *key, const void *datum)
{
  return strcmp ((char *) key, ((element_node *) datum)->symbol);
}


/*------------------------------------------------------------*/
int
element_number (const char *symbol)
{
  element_node *es;

  assert (symbol);
  assert (*symbol);

  if (sorted_element_nodes == NULL) initialize();
  es = bsearch (symbol, sorted_element_nodes, element_max_number,
		sizeof (element_node), bsearch_compare);
  if (es) {
    return es->number;
  } else {
    return 0;
  }
}


/*------------------------------------------------------------*/
int
element_number_convert (const char *symbol)
{
  char proper[3];
  char c;

  assert (symbol);
  assert (*symbol);

  c = symbol[0];
  if (isalpha (c)) {
    proper[0] = toupper (c);
    c = symbol[1];
    if (c && isalpha (c)) {
      proper[1] = tolower (c);
      proper[2] = '\0';
    } else {
      proper[1] = '\0';
    }
  } else {
    c = symbol[1];
    if (c && isalpha (c)) {
      proper[0] = c;
      proper[1] = '\0';
    } else {
      return 0;
    }
  }

  return element_number (proper);
}


/*------------------------------------------------------------*/
int
element_valid_number (int number)
{
  return ((number > 0) && (number <= element_max_number));
}


/*------------------------------------------------------------*/
char *
element_symbol (int element_number)
{
  if (element_valid_number (element_number)) {
    return element_nodes[element_number-1].symbol;
  } else {
    return NULL;
  }
}
