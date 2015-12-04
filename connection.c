#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "connection.h"
#include "socket_utils.h"

struct connection {
	int fd;
	char local_ip[ip_buf_size];
	int local_port;
	char remote_ip[ip_buf_size];
	int remote_port;
};

return_code connection_create(connection **result,
	const char *host, int port)
{
	char portbuf[22]; // enough for 64 bits
	sprintf(portbuf, "%d", port); 	
	
	struct addrinfo *info;
	struct addrinfo hints;

	memset(&hints, '\0', sizeof hints);
	hints.ai_flags = AI_NUMERICSERV;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	int r = getaddrinfo(host, portbuf, &hints, &info);
	if (r != 0) {
		return cant_resolve_host;
	}

	int fd = -1;
	return_code rc = cant_connect;

	struct addrinfo *node;
	for (node = info; node != NULL; node = node->ai_next) {

		fd = socket(node->ai_family,
			node->ai_socktype | SOCK_CLOEXEC,
			node->ai_protocol);
		if (fd == -1) {
			rc = cant_create_socket;
			break;
		}

		r = connect(fd, node->ai_addr, node->ai_addrlen);
		if (r == 0) {
			rc = ok;
			break;
		}
		
		close(fd);
		fd = -1;
	}

	freeaddrinfo(info);

	if (fd == -1) {
		return rc;
	}

	return connection_consume_internal(result, fd);
}

return_code connection_consume_internal(connection **result, int fd)
{
	set_nonblocking(fd);
	set_keepalive(fd);
	disable_nagle(fd);

	connection *conn = malloc(sizeof *conn);
	if (conn == NULL) {
		close(fd);
		return out_of_memory;
	}

	conn->fd = fd;
	get_local_ip_address(conn->local_ip, fd);
	conn->local_port = get_local_port_number(fd);
	get_remote_ip_address(conn->remote_ip, fd);
	conn->remote_port = get_remote_port_number(fd);

	*result = conn;
	return ok;
}	

const char *connection_local_ip(const connection *conn)
{
	return conn->local_ip;
}

int connection_local_port(const connection *conn)
{
	return conn->local_port;
}

const char *connection_remote_ip(const connection *conn)
{
	return conn->remote_ip;
}

int connection_remote_port(const connection *conn)
{
	return conn->remote_port;
}

void connection_activate_io_slot(connection *conn, 
	dispatcher *disp, io_slot *slot, 
	io_mode mode, return_code (*callback)(void *), void *callback_arg)
{
	dispatcher_activate_io_slot(disp, slot, conn->fd, mode,
		callback, callback_arg);
}

return_code connection_send_blocking(connection *conn,
	int *bytes_sent, const char *data, int max_bytes)
{
	return_code rc;

	while ((rc = connection_send_nonblocking(conn, bytes_sent,
		data, max_bytes)) == would_block) {
		await_output(conn->fd);
	}

	return rc;
}

return_code connection_send_nonblocking(connection *conn,
	int *bytes_sent, const char *data, int max_bytes)
{
	int r = send(conn->fd, data, max_bytes, MSG_NOSIGNAL);
	if (r == -1) {
		return errno == EAGAIN || errno == EWOULDBLOCK ?
			would_block : cant_send;
	}

	*bytes_sent = r;
	return ok;
}

return_code connection_receive_blocking(connection *conn,
	int *bytes_received, char *data, int max_bytes)
{
	return_code rc;

	while ((rc = connection_receive_nonblocking(conn, bytes_received,
		data, max_bytes)) == would_block) {
		await_input(conn->fd);
	}

	return rc;
}

return_code connection_receive_nonblocking(connection *conn,
	int *bytes_received, char *data, int max_bytes)
{
	int r = recv(conn->fd, data, max_bytes, 0);
	if (r == -1) {
		return errno == EAGAIN || errno == EWOULDBLOCK ?
			would_block : cant_receive;
	}

	*bytes_received = r;
	return ok;
}

void connection_destroy(connection *conn)
{
	close(conn->fd);
	free(conn);
}
