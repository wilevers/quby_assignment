#ifndef MAP_H
#define MAP_H

#include "return_code.h"

typedef struct map map;

return_code map_create(map **result);

return_code map_set_value(map *m, const char *key, const char *value);
int map_get_n_keys(const map *m);
const char *map_get_key(const map *m, int idx);
const char *map_get_value(const map *m, int idx);
const char *map_find_value(const map *m, const char *key);

void map_destroy(map *m);

#endif
