#ifndef PUSH_LEXER_H
#define PUSH_LEXER_H

#include "return_code.h"

typedef struct push_lexer push_lexer;

typedef struct {
	return_code (*on_character_data)(void *target_object,
		const char *data);
	return_code (*on_open_element)(void *target_object,
		const char *tag);
	return_code (*on_close_element)(void *target_object,
		const char *tag);
} push_lexer_vtbl;

return_code push_lexer_create(push_lexer **result,
	void *target_object, const push_lexer_vtbl *vtbl);
return_code push_lexer_push(push_lexer *lexer,
	const char *src, int src_length);
void push_lexer_destroy(push_lexer *lexer);

#endif
