#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "connection.h"
#include "data_session.h"
#include "lprintf.h"
#include "message_buffer.h"
#include "push_parser.h"

typedef enum {
	message_type_none,
	message_type_update,
	message_type_retrieve
} message_type;

struct data_session {
	dispatcher *disp;
	data_store *store;
	connection *conn;
	io_slot *input_slot;
	push_parser *parser;
	message_type curr_message_type;
	map *curr_message_map;
	io_slot *output_slot;
	message_buffer *output_buffer;
};

return_code on_input(void *user_data)
{
	data_session *sess = user_data;

	char buf[1024];

	int bytes_received;
	return_code rc = connection_receive_nonblocking(sess->conn,
		&bytes_received, buf, sizeof buf);

	switch (rc) {
	case ok :
		if (bytes_received == 0) {
			lprintf(info, "session %s %d <-> %s %d: %s\n",
				connection_local_ip(sess->conn),
				connection_local_port(sess->conn),
				connection_remote_ip(sess->conn),
				connection_remote_port(sess->conn),
				"disconnected by peer"
			);
			data_store_stop_session(sess->store, sess);
			return ok;
		}

		rc = push_parser_push(sess->parser, buf, bytes_received);
		if (rc != ok) {
			lprintf(error, "session %s %d <-> %s %d: %s\n",
				connection_local_ip(sess->conn),
				connection_local_port(sess->conn),
				connection_remote_ip(sess->conn),
				connection_remote_port(sess->conn),
				return_code_string(rc)
			);
			data_store_stop_session(sess->store, sess);
			return ok;
		}
			
		break;

	case would_block :
		/* Thank you very much */
		break;

	default :
		lprintf(warning, "session %s %d <-> %s %d: %s\n",
			connection_local_ip(sess->conn),
			connection_local_port(sess->conn),
			connection_remote_ip(sess->conn),
			connection_remote_port(sess->conn),
			return_code_string(rc)
		);
		data_store_stop_session(sess->store, sess);
		return ok;
	}

	if (message_buffer_size(sess->output_buffer) == 0) {
		connection_activate_io_slot(sess->conn, sess->disp,
			sess->input_slot, input, &on_input, sess);
	}

	return ok;
}		

return_code on_output(void *user_data)
{
	data_session *sess = user_data;

	int size = message_buffer_size(sess->output_buffer);
	if (size != 0) {

		int bytes_sent;
		return_code rc = connection_send_nonblocking(sess->conn, 
			&bytes_sent,
			message_buffer_data(sess->output_buffer),
			size);

		switch (rc) {
		case ok :

			message_buffer_discard(sess->output_buffer,
				bytes_sent);
			break;

		case would_block :

			/* maybe next time? */
			break;

		default :
			lprintf(error, "session %s %d <-> %s %d: %s\n",
				connection_local_ip(sess->conn),
				connection_local_port(sess->conn),
				connection_remote_ip(sess->conn),
				connection_remote_port(sess->conn),
				return_code_string(rc));
			data_store_stop_session(sess->store, sess);
			return ok;
		}

		size = message_buffer_size(sess->output_buffer);
	}

	if (size != 0) {
		connection_activate_io_slot(sess->conn, sess->disp,
			sess->output_slot, output, &on_output, sess);
	} else {
		connection_activate_io_slot(sess->conn, sess->disp,
			sess->input_slot, input, &on_input, sess);
	}

	return ok;
}
		
static return_code send_status(data_session *sess, const map *query)
{
	int was_sending = message_buffer_size(sess->output_buffer) != 0;

	return_code rc = message_buffer_add_begin_message(
		sess->output_buffer, "status");
	if (rc != ok) {
		return rc;
	}

	const map *store_data = data_store_data(sess->store);
	int n_store_keys = map_get_n_keys(store_data);
	int query_empty = map_get_n_keys(query) == 0;

	int i;
	for (i = 0; i != n_store_keys; ++i) {

		const char *store_key = map_get_key(store_data, i);
		if (query_empty || map_find_value(query, store_key) != NULL) {

			rc = message_buffer_add_string_value(
				sess->output_buffer, 
				store_key,
				map_get_value(store_data, i));
			if (rc != ok) {
				return rc;
			}
		}
	}	

	rc = message_buffer_add_end_message(sess->output_buffer, "status");
	if (rc != ok) {
		return rc;
	}

	if (! was_sending) {
		connection_activate_io_slot(sess->conn, sess->disp,
			sess->output_slot, output, &on_output, sess);
	}
	
	return ok;
}	

static return_code on_begin_message(void *target_object, const char *type)
{
	data_session *sess = target_object;

	assert(sess->curr_message_type == message_type_none);

	if (strcmp(type, "update") == 0) {
		sess->curr_message_type = message_type_update;
	} else if (strcmp(type, "retrieve") == 0) {
		sess->curr_message_type = message_type_retrieve;
	} else {
		return invalid_message_type;
	}
		
	return ok;
}

static return_code on_message_data(void *target_object,
	const char *key, const char *data)
{
	data_session *sess = target_object;
	return_code rc;

	switch (sess->curr_message_type) {

	case message_type_update :

		rc = map_set_value(sess->curr_message_map, key, data);
		if (rc != ok) {
			return rc;
		}

		break;

	case message_type_retrieve :

		if (strcmp(key, "key") != 0) {
			return key_expected;
		}

		rc = map_set_value(sess->curr_message_map, data, "");
		if (rc != ok) {
			return rc;
		}
		break;

	default :

		assert(0);
		break;
	}

	return ok;
}

static return_code on_end_message(void *target_object)
{
	data_session *sess = target_object;
	return_code rc;

	switch (sess->curr_message_type) {

	case message_type_update :

		rc = data_store_update(sess->store, sess->curr_message_map);
		if (rc != ok) {
			return rc;
		}

		lprintf(info, "session %s %d <-> %s %d: %s\n",
			connection_local_ip(sess->conn),
			connection_local_port(sess->conn),
			connection_remote_ip(sess->conn),
			connection_remote_port(sess->conn),
			"some data values updated");
		break;

	case message_type_retrieve :

		rc = send_status(sess, sess->curr_message_map);
		if (rc != ok) {
			return rc;
		}

		lprintf(info, "session %s %d <-> %s %d: %s\n",
			connection_local_ip(sess->conn),
			connection_local_port(sess->conn),
			connection_remote_ip(sess->conn),
			connection_remote_port(sess->conn),
			"sending status");
		break;

	default :
		 
		assert(0);
		break;
	}

	map_clear(sess->curr_message_map);
	sess->curr_message_type = message_type_none;

	return ok;
}

static const push_parser_vtbl parser_vtbl = {
	&on_begin_message,
	&on_message_data,
	&on_end_message
};

static void data_session_dispose(data_session *sess)
{
	if (sess->output_buffer != NULL) {
		message_buffer_destroy(sess->output_buffer);
	}
	if (sess->output_slot != NULL) {
		dispatcher_destroy_io_slot(sess->disp, sess->output_slot);
	}
	if (sess->curr_message_map != NULL) {
		map_destroy(sess->curr_message_map);
	}
	if (sess->parser != NULL) {
		push_parser_destroy(sess->parser);
	}
	if (sess->input_slot != NULL) {
		dispatcher_destroy_io_slot(sess->disp, sess->input_slot);
	}
	if (sess->conn != NULL) {
		connection_destroy(sess->conn);
	}

	free(sess);
}

return_code data_session_create(data_session **result,
	dispatcher *disp, data_store *store, acceptor *acc)
{
	data_session *sess = malloc(sizeof *sess);
	if (sess == NULL) {
		return out_of_memory;
	}

	sess->disp = disp;
	sess->store = store;
	sess->conn = NULL;
	sess->input_slot = NULL;
	sess->parser = NULL;
	sess->curr_message_type = message_type_none;
	sess->curr_message_map = NULL;
	sess->output_slot = NULL;
	sess->output_buffer = NULL;

	return_code rc = acceptor_accept_nonblocking(acc, &sess->conn);
	if (rc != ok) {
		data_session_dispose(sess);
		return rc;
	}

	rc = dispatcher_create_io_slot(sess->disp, &sess->input_slot);
	if (rc != ok) {
		data_session_dispose(sess);
		return rc;
	}

	rc = push_parser_create(&sess->parser, sess, &parser_vtbl);
	if (rc != ok) {
		data_session_dispose(sess);
		return rc;
	} 

	rc = map_create(&sess->curr_message_map);
	if (rc != ok) {
		data_session_dispose(sess);
		return rc;
	}
		
	rc = dispatcher_create_io_slot(sess->disp, &sess->output_slot);
	if (rc != ok) {
		data_session_dispose(sess);
		return rc;
	}

	rc = message_buffer_create(&sess->output_buffer);
	if (rc != ok) {
		data_session_dispose(sess);
		return rc;
	}
	
	connection_activate_io_slot(sess->conn, sess->disp,
		sess->input_slot, input, &on_input, sess);
	
	lprintf(info, "new session %s %d <-> %s %d\n",
		connection_local_ip(sess->conn),
		connection_local_port(sess->conn),
		connection_remote_ip(sess->conn),
		connection_remote_port(sess->conn)
	);
		
	*result = sess;
	return ok;
}

void data_session_destroy(data_session *sess)
{
	lprintf(info, "closing session %s %d <-> %s %d\n",
		connection_local_ip(sess->conn),
		connection_local_port(sess->conn),
		connection_remote_ip(sess->conn),
		connection_remote_port(sess->conn)
	);

	data_session_dispose(sess);
}
