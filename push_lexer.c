#include <assert.h>
#include <stdlib.h>

#include "push_lexer.h"

typedef enum {
	in_character_data,
	lt_seen,
	in_open_element,
	slash_seen,
	in_close_element
} push_lexer_state;

struct push_lexer {
	void *target_object;
	const push_lexer_vtbl *vtbl;
	push_lexer_state state;
	char *data_buffer;
	int data_buffer_length;
	int data_buffer_alloc;
};

static int is_whitespace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static return_code append_char(push_lexer *lexer, char c)
{
	if (lexer->data_buffer_length == lexer->data_buffer_alloc) {

		int new_alloc = lexer->data_buffer_alloc +
			lexer->data_buffer_alloc / 2 + 1;
		char *new_data_buffer = lexer->data_buffer == NULL ?
			malloc(new_alloc) :
			realloc(lexer->data_buffer, new_alloc);

		if (new_data_buffer == NULL) {
			return out_of_memory;
		}

		lexer->data_buffer = new_data_buffer;
		lexer->data_buffer_alloc = new_alloc;
	}

	lexer->data_buffer[lexer->data_buffer_length] = c;
	++lexer->data_buffer_length;

	return ok;
}

static return_code report_character_data(push_lexer *lexer)
{
	int begin;
	for (begin = 0; begin != lexer->data_buffer_length; ++begin) {
		if (! is_whitespace(lexer->data_buffer[begin])) {
			break;
		}
	}

	int end;
	for (end = lexer->data_buffer_length; end != begin; --end) {
		if (! is_whitespace(lexer->data_buffer[end - 1])) {
			break;
		}
	}

	if (begin != end) {

		return_code rc;

		if (end == lexer->data_buffer_length) {
			rc = append_char(lexer, '\0');
			if (rc != ok) {
				return rc;
			}
		} else {
			lexer->data_buffer[end] = '\0';
		}

		rc = (*lexer->vtbl->on_character_data)(
			lexer->target_object, lexer->data_buffer + begin);
		if (rc != ok) {
			return rc;
		}
	}

	lexer->data_buffer_length = 0;
	return ok;
}

static return_code report_open_element(push_lexer *lexer)
{
	return_code rc = append_char(lexer, '\0');
	if (rc != ok) {
		return rc;
	}

	rc = (*lexer->vtbl->on_open_element)(
		lexer->target_object, lexer->data_buffer);
	if (rc != ok) {
		return rc;
	}

	lexer->data_buffer_length = 0;
	return ok;
}
	
static return_code report_openclose_element(push_lexer *lexer)
{
	return_code rc = append_char(lexer, '\0');
	if (rc != ok) {
		return rc;
	}

	rc = (*lexer->vtbl->on_open_element)(
		lexer->target_object, lexer->data_buffer);
	if (rc != ok) {
		return rc;
	}

	rc = (*lexer->vtbl->on_close_element)(
		lexer->target_object, lexer->data_buffer);
	if (rc != ok) {
		return rc;
	}

	lexer->data_buffer_length = 0;
	return ok;
}
	
static return_code report_close_element(push_lexer *lexer)
{
	return_code rc = append_char(lexer, '\0');
	if (rc != ok) {
		return rc;
	}

	rc = (*lexer->vtbl->on_close_element)(
		lexer->target_object, lexer->data_buffer);
	if (rc != ok) {
		return rc;
	}

	lexer->data_buffer_length = 0;
	return ok;
}
	
static return_code process_char(push_lexer *lexer, char c)
{
	if (c == '\0') {
		return unexpected_null_char;
	}

	return_code rc;

	switch (lexer->state) {

	case in_character_data :

		switch (c) {
		case '<' :
			rc = report_character_data(lexer);
			if (rc != ok) {
				return rc;
			}
			lexer->state = lt_seen;
			break;
		case '>' :
			return unexpected_gt;
			break;
		default :
			rc = append_char(lexer, c);
			if (rc != ok) {
				return rc;
			}
			break;
		}
		break;

	case lt_seen :

		switch (c) {
		case '/' :
			lexer->state = in_close_element;
			break;
		default :
			lexer->state = in_open_element;
			rc = process_char(lexer, c);
			if (rc != ok) {
				return rc;
			}
			break;
		}
		break;

	case in_open_element :

		switch (c) {
		case '/' :
			lexer->state = slash_seen;
			break;
		case '>' :
			rc = report_open_element(lexer);
			if (rc != ok) {
				return rc;
			}
			lexer->state = in_character_data;
			break;
		default :
			if (is_whitespace(c)) {
				return unexpected_whitespace;
			}
			rc = append_char(lexer, c);
			if (rc != ok) {
				return rc;
			}
			break;
		}
		break;
		
	case slash_seen :

		switch (c) {
		case '>' :
			rc = report_openclose_element(lexer);
			if (rc != ok) {
				return rc;
			}
			lexer->state = in_character_data;
			break;
		default :
			return gt_expected;
			break;
		}
		break;

	case in_close_element :

		switch (c) {
		case '/' :
			return unexpected_slash;
			break;
		case '>' :
			rc = report_close_element(lexer);
			if (rc != ok) {
				return rc;
			}
			lexer->state = in_character_data;
			break;
		default :
			if (is_whitespace(c)) {
				return unexpected_whitespace;
			}
			rc = append_char(lexer, c);
			if (rc != ok) {
				return rc;
			}
			break;
		}
		break;

	default :
		assert(0);
		break;
	}

	return ok;
}

return_code push_lexer_create(push_lexer **result,
	void *target_object, const push_lexer_vtbl *vtbl)
{
	push_lexer *lexer = malloc(sizeof *lexer);
	if (lexer == NULL) {
		return out_of_memory;
	}

	lexer->target_object = target_object;
	lexer->vtbl = vtbl;
	lexer->state = in_character_data;
	lexer->data_buffer = NULL;
	lexer->data_buffer_length = 0;
	lexer->data_buffer_alloc = 0;

	*result = lexer;
	return ok;
}

return_code push_lexer_push(push_lexer *lexer,
	const char *src, int src_length)
{
	const char *end = src + src_length;

	while (src != end) {
		return_code rc = process_char(lexer, *src);
		if (rc != ok) {
			return rc;
		}
		++src;
	}

	return ok;
}

void push_lexer_destroy(push_lexer *lexer)
{
	free(lexer->data_buffer);
	free(lexer);
}
