#ifndef MESSAGE_BUFFER_H
#define MESSAGE_BUFFER_H

#include "return_code.h"

typedef struct message_buffer message_buffer;

return_code message_buffer_create(message_buffer **result);

return_code message_buffer_add_begin_message(message_buffer *buf, 
	const char *message_type);
return_code message_buffer_add_string_value(message_buffer *buf,
	const char *key, const char *value);
return_code message_buffer_add_integer_value(message_buffer *buf,
	const char *key, int value);
return_code message_buffer_add_end_message(message_buffer *buf,
	const char *message_type);

const char *message_buffer_data(const message_buffer *buf);
int message_buffer_size(const message_buffer *buf);
void message_buffer_discard(message_buffer *buf, int n_bytes);

void message_buffer_destroy(message_buffer *buf);

#endif
