/* for accept4() */
#define _GNU_SOURCE 

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "acceptor.h"
#include "socket_utils.h"

struct acceptor {
	int fd;
	char ip[ip_buf_size];
	int port;
};

return_code acceptor_create(acceptor **result,
	const char *ip_address, int port)
{
	char portbuf[22]; // enough for 64 bits
	sprintf(portbuf, "%d", port); 	
	
	struct addrinfo *info;
	struct addrinfo hints;

	memset(&hints, '\0', sizeof hints);
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	int r = getaddrinfo(ip_address, portbuf, &hints, &info);
	if (r != 0) {
		return cant_resolve_ip;
	}

	assert(info != NULL);
	int fd = socket(info->ai_family,
		info->ai_socktype | SOCK_CLOEXEC,
		info->ai_protocol);
	if (fd == -1) {
		freeaddrinfo(info);
		return cant_create_socket;
	}

	set_reuseaddress(fd);
	r = bind(fd, info->ai_addr, info->ai_addrlen);
	if (r == -1) {
		close(fd);
		freeaddrinfo(info);
		return cant_bind;
	}

	freeaddrinfo(info);

	r = listen(fd, SOMAXCONN);
	if (r == -1) {
		close(fd);
		return cant_listen;
	}

	set_nonblocking(fd);

	acceptor *acc = malloc(sizeof *acc);
	if (acc == NULL) {
		close(fd);
		return out_of_memory;
	}

	acc->fd = fd;
	get_local_ip_address(acc->ip, fd);
	acc->port = get_local_port_number(fd);

	*result = acc;
	return ok;
}

const char *acceptor_ip(const acceptor *acc)
{
	return acc->ip;
}

int acceptor_port(const acceptor *acc)
{
	return acc->port;
}

void acceptor_activate_io_slot(acceptor *acc,
	dispatcher *disp, io_slot *slot,
	return_code (*callback)(void *), void *callback_arg)
{
	dispatcher_activate_io_slot(disp, slot,
		acc->fd, input, callback, callback_arg);
}

return_code acceptor_accept_blocking(acceptor *acc, connection **result)
{
	return_code rc;

	while ((rc = acceptor_accept_nonblocking(acc, result)) == 
		would_block) {
		await_input(acc->fd);
	}

	return rc;
}

return_code acceptor_accept_nonblocking(acceptor *acc, connection **result)
{
	int fd = accept4(acc->fd, NULL, NULL, SOCK_CLOEXEC);
	if (fd == -1) {
/*
		return errno == EAGAIN || errno == EWOULDBLOCK ?
			would_block : cant_accept;

		Follow Linux man page advice: treat every failure here
		as non-fatal for the acceptor:
 */
		return would_block;
	}

	return connection_consume_internal(result, fd);
}
		
void acceptor_destroy(acceptor *acc)
{
	close(acc->fd);
	free(acc);
}

	