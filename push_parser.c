#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "push_lexer.h"
#include "push_parser.h"

struct push_parser {
	void *target_object;
	const push_parser_vtbl *vtbl;
	push_lexer *lexer;
	int nesting_level;
	char *current_message_type; /* set when nesting_level >= 1 */
	char *current_key; /* set when nesting_level >= 2 */
};

static return_code on_character_data(void *target_object, const char *data)
{	
	push_parser *parser = target_object;

	switch (parser->nesting_level) {
	case 0 :
	case 1 :
		return unexpected_character_data;
		break;
	case 2 :
		assert(parser->current_key != NULL);
		return (*parser->vtbl->on_message_data)(
			parser->target_object, parser->current_key, data);
		break;
	default :
		assert(0);
		break;
	}

	return ok;
}

static return_code on_open_element(void *target_object, const char *tag)
{
	push_parser *parser = target_object;
	
	switch (parser->nesting_level) {
	case 0 :
		assert(parser->current_message_type == NULL);
		parser->current_message_type = malloc(strlen(tag) + 1);
		if (parser->current_message_type == NULL) {
			return out_of_memory;
		} 
		strcpy(parser->current_message_type, tag);
		++parser->nesting_level;
		return (*parser->vtbl->on_begin_message)(
			parser->target_object, parser->current_message_type);
		break;
	case 1 :
		assert(parser->current_key == NULL);
		parser->current_key = malloc(strlen(tag) + 1);
		if (parser->current_key == NULL) {
			return out_of_memory;
		}
		strcpy(parser->current_key, tag);
		++parser->nesting_level;
		break;
	case 2 :
		return unexpected_begin_element;
		break;
	default :
		assert(0);
		break;
	}

	return ok;
}

static return_code on_close_element(void *target_object, const char *tag)
{
	push_parser *parser = target_object;

	switch (parser->nesting_level) {
	case 0 :
		return unexpected_end_element;
		break;
	case 1 :
		assert(parser->current_message_type != NULL);
		if (strcmp(parser->current_message_type, tag) != 0) {
			return message_type_mismatch;
		}
		free(parser->current_message_type);
		parser->current_message_type = NULL;
		--parser->nesting_level;
		return (*parser->vtbl->on_end_message)(parser->target_object);
		break;
	case 2 :
		assert(parser->current_key != NULL);
		if (strcmp(parser->current_key, tag) != 0) {
			return data_key_mismatch;
		}
		free(parser->current_key);
		parser->current_key = NULL;
		--parser->nesting_level;
		break;
	default :
		assert(0);
		break;
	}
		
	return ok;
}

static const push_lexer_vtbl lexer_vtbl = {
	&on_character_data,
	&on_open_element,
	&on_close_element
};
	
return_code push_parser_create(push_parser **result,
	void *target_object, const push_parser_vtbl *vtbl)
{
	push_parser *parser = malloc(sizeof *parser);
	if (parser == NULL) {
		return out_of_memory;
	}

	parser->target_object = target_object;
	parser->vtbl = vtbl;

	return_code rc = push_lexer_create(
		&parser->lexer, parser, &lexer_vtbl);
	if (rc != ok) {
		free(parser);
		return rc;
	}
	
	parser->nesting_level = 0;
	parser->current_message_type = NULL;
	parser->current_key = NULL;
	
	*result = parser;
	return ok;
}

return_code push_parser_push(push_parser *parser,
	const char *src, int src_length)
{
	return push_lexer_push(parser->lexer, src, src_length);
}
	
void push_parser_destroy(push_parser *parser)
{
	free(parser->current_key);
	free(parser->current_message_type);
	push_lexer_destroy(parser->lexer);
	free(parser);
}
