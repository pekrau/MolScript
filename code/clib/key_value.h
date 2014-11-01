#ifndef KEY_VALUE_H
#define KEY_VALUE_H 1

#include <dynstring.h>

typedef struct s_key_value key_value;

struct s_key_value {
  dynstring *key;
  dynstring *value;
  key_value *next;
};

key_value *
kv_create (const char *k, const char *v);

void
kv_delete (key_value *kv);

key_value *
kv_append (key_value *kv, key_value *new);

key_value *
kv_search (key_value *first, const char *key);

char *
kv_search_value_str (key_value *first, char *key);

#endif
