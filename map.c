#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "return_code.h"

typedef struct {
	char *key;
	char *value;
} kvpair;
	
struct map {
	kvpair *kvpairs;
	int n_kvpairs_used;
	int n_kvpairs_alloc;
};

static int is_valid_key(const char *key)
{
	if (*key == '\0') {
		return 0;
	}

	if (! isalpha(*key)) {
		return 0;
	}

	for (++key; *key != '\0'; ++key) {
		if (! isalnum(*key) && *key != '_') {
			return 0;
		}
	}

	return 1;
}

return_code map_create(map **result)
{
	map *m = malloc(sizeof *m);
	if (m == NULL) {
		return out_of_memory;
	}

	m->kvpairs = NULL;
	m->n_kvpairs_used = 0;
	m->n_kvpairs_alloc = 0;

	*result = m;
	return ok;
}

return_code map_set_value(map *m, const char *key, const char *value)
{
	int i;
	for (i = 0; i != m->n_kvpairs_used; ++i) {
		if (strcmp(m->kvpairs[i].key, key) == 0) {
			break;
		}
	}

	if (i == m->n_kvpairs_alloc) {

		int new_alloc = m->n_kvpairs_alloc +
			m->n_kvpairs_alloc / 2 + 1;
		kvpair *new_kvpairs = m->kvpairs == NULL ?
			malloc(sizeof *new_kvpairs * new_alloc) :
			realloc(m->kvpairs,
				sizeof *new_kvpairs * new_alloc);
		if (new_kvpairs == NULL) {
			return out_of_memory;
		}
		
		m->kvpairs = new_kvpairs;
		m->n_kvpairs_alloc = new_alloc;
	}

	char *new_key = NULL;
	if (i == m->n_kvpairs_used) {
		
		if (! is_valid_key(key)) {
			return invalid_map_key;
		}

		new_key = malloc(strlen(key) + 1);
		if (new_key == NULL) {
			return out_of_memory;
		}
		strcpy(new_key, key);
	}

	char *new_value = malloc(strlen(value) + 1);
	if (new_value == NULL) {
		free(new_key);
		return out_of_memory;
	}
	strcpy(new_value, value);

	if (new_key == NULL) {
		free(m->kvpairs[i].value);
		m->kvpairs[i].value = new_value;
	} else {
		m->kvpairs[i].key = new_key;
		m->kvpairs[i].value = new_value;
		++m->n_kvpairs_used;
	}

	return ok;
}
	
int map_get_n_keys(const map *m)
{
	return m->n_kvpairs_used;
}

const char *map_get_key(const map *m, int idx)
{
	assert(idx >= 0);
	assert(idx < m->n_kvpairs_used);

	return m->kvpairs[idx].key;
}

const char *map_get_value(const map *m, int idx)
{
	assert(idx >= 0);
	assert(idx < m->n_kvpairs_used);
	
	return m->kvpairs[idx].value;
}

const char *map_find_value(const map *m, const char *key)
{
	int i;
	for (i = 0; i != m->n_kvpairs_used; ++i) {
		if (strcmp(m->kvpairs[i].key, key) == 0) {
			return m->kvpairs[i].value;
		}
	}

	return NULL;
}
		
void map_destroy(map *m)
{
	int i;
	for (i = 0; i != m->n_kvpairs_used; ++i) {
		free(m->kvpairs[i].key);
		free(m->kvpairs[i].value);
	}

	free(m->kvpairs);
	free(m);
}
