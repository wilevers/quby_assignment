#include <stdio.h>

#include <string.h>
#include "map.h"

#undef NDEBUG
#include <assert.h>

static void map_create_test()
{
	map *m = NULL;

	return_code rc = map_create(&m);
	assert(rc == ok);
	assert(m != NULL);
	
	map_destroy(m);
}

static void map_set_value_test()
{
	map *m;
	return_code rc = map_create(&m);
	assert(rc == ok);
	assert(map_get_n_keys(m) == 0);

	assert(map_find_value(m, "key1") == NULL);
	rc = map_set_value(m, "key1", "value1");
	assert(rc == ok);
	assert(map_get_n_keys(m) == 1);
	assert(strcmp(map_get_key(m, 0), "key1") == 0);
	assert(strcmp(map_get_value(m, 0), "value1") == 0);
	const char *val = map_find_value(m, "key1");
	assert(val != NULL);
	assert(strcmp(val, "value1") == 0);
	
	assert(map_find_value(m, "key2") == NULL);
	rc = map_set_value(m, "key2", "value2");
	assert(rc == ok);
	assert(map_get_n_keys(m) == 2);
	val = map_find_value(m, "key2");
	assert(val != NULL);
	assert(strcmp(val, "value2") == 0);
	
	rc = map_set_value(m, "key2", "value3");
	assert(rc == ok);
	assert(map_get_n_keys(m) == 2);
	val = map_find_value(m, "key2");
	assert(val != NULL);
	assert(strcmp(val, "value3") == 0);

	map_destroy(m);
}

static void invalid_key_test()
{
	map *m;
	return_code rc = map_create(&m);
	assert(rc == ok);

	rc = map_set_value(m, "", "value");
	assert(rc == invalid_map_key);

	map_destroy(m);
}
	
int main()
{
	map_create_test();
	map_set_value_test();
	invalid_key_test();

	return 0;
}
