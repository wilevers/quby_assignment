#ifndef CONNECTION_H
#define CONNECTION_H

#include "dispatcher.h"
#include "return_code.h"

typedef struct connection connection;

return_code connection_create(connection **result,
	const char *host, int port);

/* connection_consume_internal is for internal use by acceptor only */
return_code connection_consume_internal(connection **result, int fd);

const char *connection_local_ip(const connection *conn);
int connection_local_port(const connection *conn);
const char *connection_remote_ip(const connection *conn);
int connection_remote_port(const connection *conn);

void connection_activate_io_slot(connection *conn,
	dispatcher *disp, io_slot *slot,
	io_mode mode, void (*callback)(void *), void *user_data);

return_code connection_send_blocking(connection *conn,
	int *bytes_sent, const char *data, int max_bytes);
return_code connection_send_nonblocking(connection *conn,
	int *bytes_sent, const char *data, int max_bytes);

return_code connection_receive_blocking(connection *conn,
	int *bytes_received, char *data, int max_bytes);
return_code connection_receive_nonblocking(connection *conn,
	int *bytes_received, char *data, int max_bytes);

void connection_destroy(connection *conn);

#endif
