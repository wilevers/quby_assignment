#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include "dispatcher.h"
#include "return_code.h"

typedef struct data_source data_source;

return_code data_source_create(data_source **result,
	dispatcher *disp, const char *target_host, int target_port);

return_code data_source_set_string_value(data_source *src,
	const char *key, const char *value);
return_code data_source_set_integer_value(data_source *src,
	const char *key, int value);

void data_source_destroy(data_source *src);

#endif
