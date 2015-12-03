#ifndef PUSH_PARSER_H
#define PUSH_PARSER_H

#include "return_code.h"

typedef struct push_parser push_parser;

typedef struct {
	return_code (*on_begin_message)(void *target_object,
		const char *type);
	return_code (*on_message_data)(void *target_object,
		const char *key, const char *data);
	return_code (*on_end_message)(void *target_object);
} push_parser_vtbl;

return_code push_parser_create(push_parser **result,
	void *target_object,
	const push_parser_vtbl *vtbl
);

return_code push_parser_push(push_parser *parser,
	const char *src, int src_length);

void push_parser_destroy(push_parser *parser);

#endif
