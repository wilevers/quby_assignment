#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "message_buffer.h"

struct message_buffer {
	char *data;
	int data_size;
	int write_index;
	int read_index;
};

static return_code message_buffer_add(message_buffer *buf, const char *str)
{
	while (*str != '\0') {

		if (buf->write_index == buf->data_size) {

			int new_data_size = buf->data_size + 
				buf->data_size / 2 + 1;
			char *new_data = buf->data == NULL ?
				malloc(new_data_size) :
				realloc(buf->data, new_data_size);

			if (new_data == NULL) {
				return out_of_memory;
			}

			buf->data_size = new_data_size;
			buf->data = new_data;
		}

		buf->data[buf->write_index] = *str;
		++buf->write_index;

		++str;
	}

	return ok;
}

static return_code message_buffer_add_begin_element(message_buffer *buf,
	const char *name)
{
	return_code rc = message_buffer_add(buf, "<");
	if (rc != ok) {
		return rc;
	}

	rc = message_buffer_add(buf, name);
	if (rc != ok) {
		return rc;
	}

	return message_buffer_add(buf, ">");
}
	
static return_code message_buffer_add_end_element(message_buffer *buf,
	const char *name)
{
	return_code rc = message_buffer_add(buf, "</");
	if (rc != ok) {
		return rc;
	}

	rc = message_buffer_add(buf, name);
	if (rc != ok) {
		return rc;
	}

	return message_buffer_add(buf, ">");
}
	
return_code message_buffer_create(message_buffer **result)
{
	message_buffer *buf = malloc(sizeof *buf);
	if (buf == NULL) {
		return out_of_memory;
	}

	buf->data = NULL;
	buf->data_size = 0;
	buf->write_index = 0;
	buf->read_index = 0;

	*result = buf;
	return ok;
}

return_code message_buffer_add_begin_message(message_buffer *buf,
	const char *message_type)
{
	return_code rc = message_buffer_add_begin_element(buf, message_type);
	if (rc != ok) {
		return rc;
	}

	return message_buffer_add(buf, "\n");
}

return_code message_buffer_add_string_value(message_buffer *buf,
	const char *key, const char *value)
{
	return_code rc = message_buffer_add(buf, "\t");
	if (rc != ok) {
		return rc;
	}

	rc = message_buffer_add_begin_element(buf, key);
	if (rc != ok) {
		return rc;
	}

	rc = message_buffer_add(buf, value);
	if (rc != ok) {
		return rc;
	}

	rc = message_buffer_add_end_element(buf, key);
	if (rc != ok) {
		return rc;
	}

	return message_buffer_add(buf, "\n");
}

return_code message_buffer_add_integer_value(message_buffer *buf,
	const char *key, int value)
{
	char value_buf[22]; // enough for 64 bit
	sprintf(value_buf, "%d", value);
		
	return message_buffer_add_string_value(buf, key, value_buf);
}

return_code message_buffer_add_end_message(message_buffer *buf,
	const char *message_type)
{
	return_code rc = message_buffer_add_end_element(buf, message_type);
	if (rc != ok) {
		return rc;
	}

	return message_buffer_add(buf, "\n");
}

const char *message_buffer_data(const message_buffer *buf)
{
	return buf->data + buf->read_index;
}

int message_buffer_size(const message_buffer *buf)
{
	return buf->write_index - buf->read_index;
}

void message_buffer_discard(message_buffer *buf, int n_bytes)
{
	assert(n_bytes >= 0);
	assert(n_bytes <= message_buffer_size(buf));

	buf->read_index += n_bytes;
	if (buf->read_index == buf->write_index) {
		buf->write_index = 0;
		buf->read_index = 0;
	}
}
	
void message_buffer_destroy(message_buffer *buf)
{
	free(buf->data);
	free(buf);
}
