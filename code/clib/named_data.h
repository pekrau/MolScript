#ifndef NAMED_DATA_H
#define NAMED_DATA_H 1

#include <stdlib.h>

#include <boolean.h>

typedef struct _named_data named_data;

struct _named_data {
  char *name;
  void *data;
  boolean name_copy, data_copy;
  named_data *next;
};

named_data *
nd_create (char *name, boolean copy);

named_data *
nd_set_data (named_data *nd, void *data, boolean copy);

named_data *
nd_set_data_clone (named_data *nd, void *data, size_t size);

void
nd_delete (named_data *nd);

named_data *
nd_append (named_data *nd, named_data *new);

named_data *
nd_search (named_data *nd, const char *name);

void *
nd_search_data (named_data *nd, const char *name);

#endif
