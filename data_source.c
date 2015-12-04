#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "connection.h"
#include "data_source.h"
#include "lprintf.h"
#include "map.h"
#include "message_buffer.h"

enum { send_interval = 15000 };

struct data_source {
	dispatcher *disp;
	char *target_host;
	int target_port;
	map *data;
	message_buffer *buffer;
	alarm_slot *alarm; 
	io_slot *output_slot;
	connection *conn; /* NULL when sleeping, non-NULL when sending */
};

static return_code on_output(void *user_data);

static return_code on_alarm(void *user_data)
{
	data_source *src = user_data;

	assert(message_buffer_size(src->buffer) == 0);
	
	return_code rc = message_buffer_add_begin_message(
		src->buffer, "update");
	if (rc != ok) {
		return rc;
	}

	int n_data_keys = map_get_n_keys(src->data);
	int i;
	for (i = 0; i != n_data_keys; ++i) {
		rc = message_buffer_add_string_value(src->buffer,
			map_get_key(src->data, i),
			map_get_value(src->data, i));
		if (rc != ok) {
			return rc;
		}
	}

	rc = message_buffer_add_end_message(src->buffer, "update");
	if (rc != ok) {
		return rc;
	}

	assert(src->conn == NULL);
	rc = connection_create(&src->conn,
		src->target_host, src->target_port);
	if (rc != ok) {
		lprintf(warning, "data source for %s %d: %s\n",
			src->target_host, src->target_port, 
			return_code_string(rc));
		message_buffer_discard(src->buffer,
			message_buffer_size(src->buffer));
	}

	if (message_buffer_size(src->buffer) == 0) {
		dispatcher_activate_alarm_slot(src->disp,
			src->alarm, send_interval, &on_alarm, src);
	} else {
		connection_activate_io_slot(src->conn, src->disp,
			src->output_slot, output, &on_output, src);
	}

	return ok;
}

static return_code on_output(void *user_data)
{
	data_source *src = user_data;
	
	assert(src->conn != NULL);
	int size = message_buffer_size(src->buffer);
	assert(size != 0);

	int bytes_sent;
	return_code rc = connection_send_nonblocking(src->conn,
		&bytes_sent, message_buffer_data(src->buffer), size);

	switch (rc) {
	case ok :
		message_buffer_discard(src->buffer, bytes_sent);
		break;
	case would_block :
		/* huh? */
		break;
	default :
		lprintf(warning, "data source for %s %d: %s\n",
			src->target_host, src->target_port, 
			return_code_string(rc));
		message_buffer_discard(src->buffer, size);
		break;
	}

	if (message_buffer_size(src->buffer) == 0) {

		lprintf(info, "data source for %s %d: %s\n",
			src->target_host, src->target_port, 
			"updates sent");

		connection_destroy(src->conn);
		src->conn = NULL;
		dispatcher_activate_alarm_slot(src->disp,
			src->alarm, send_interval, &on_alarm, src);

	} else {

		connection_activate_io_slot(src->conn, src->disp,
			src->output_slot, output, &on_output, src);
	}

	return ok;
}
		
return_code data_source_set_string_value(data_source *src,
	const char *key, const char *value)
{
	return map_set_value(src->data, key, value);
}

return_code data_source_set_integer_value(data_source *src,
	const char *key, int value)
{
	char buf[22]; // fits for 64 bits
	sprintf(buf, "%d", value);

	return map_set_value(src->data, key, buf);
}	
	
return_code data_source_create(data_source **result,
	dispatcher *disp, const char *target_host, int target_port)
{
	data_source *src = malloc(sizeof *src);
	if (src == NULL) {
		return out_of_memory;
	}

	src->disp = disp;
	
	src->target_host = malloc(strlen(target_host) + 1);
	if (src->target_host == NULL) {
		free(src);
		return out_of_memory;
	}
	strcpy(src->target_host, target_host);

	src->target_port = target_port;

	return_code rc = map_create(&src->data);
	if (rc != ok) {
		free(src->target_host);
		free(src);
		return rc;
	}

	rc = message_buffer_create(&src->buffer);
	if (rc != ok) {
		map_destroy(src->data);
		free(src->target_host);
		free(src);
		return rc;
	}

	rc = dispatcher_create_alarm_slot(disp, &src->alarm);
	if (rc != ok) {
		message_buffer_destroy(src->buffer);
		map_destroy(src->data);
		free(src->target_host);
		free(src);
		return rc;
	}		

	rc = dispatcher_create_io_slot(disp, &src->output_slot);
	if (rc != ok) {
		dispatcher_destroy_alarm_slot(disp, src->alarm);
		map_destroy(src->data);
		free(src->target_host);
		free(src);
		return rc;
	}

	src->conn = NULL;
		
	dispatcher_activate_alarm_slot(src->disp,
		src->alarm, send_interval,
		&on_alarm, src);

	lprintf(info, "created data source for %s %d\n",
		src->target_host, src->target_port);

	*result = src;
	return ok;
}

void data_source_destroy(data_source *src)
{
	lprintf(info, "destroying data source for %s %d\n",
		src->target_host, src->target_port);

	if (src->conn != NULL) {
		connection_destroy(src->conn);
	}

	dispatcher_destroy_io_slot(src->disp, src->output_slot);
	dispatcher_destroy_alarm_slot(src->disp, src->alarm);
	message_buffer_destroy(src->buffer);
	map_destroy(src->data);
	free(src->target_host);
	free(src);
}

