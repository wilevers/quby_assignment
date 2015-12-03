#include <assert.h>
#include <stddef.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "socket_utils.h"

typedef union {
	struct sockaddr sa;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
} address_storage;

static void get_ip_address(char buf[ip_buf_size],
	const address_storage *storage, int storage_length)
{
	int r = getnameinfo(&storage->sa, storage_length,
		buf, ip_buf_size - 1, NULL, 0, NI_NUMERICHOST);
	(void) r;
	assert(r == 0);
}

static int get_port_number(const address_storage *storage, int storage_length)
{
	int port = 0;

	switch (storage->sa.sa_family) {
	case AF_INET :
		assert(storage_length == sizeof storage->sin);
		port = ntohs(storage->sin.sin_port);
		break;
	case AF_INET6 :
		assert(storage_length == sizeof storage->sin6);
		port = ntohs(storage->sin6.sin6_port);
		break;
	default :
		assert(0);
		break;
	}

	return port;
}

void set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL);
	assert(flags != -1);
	
	flags |= O_NONBLOCK;

	int r = fcntl(fd, F_SETFL, flags);
	assert(r != -1);
}

void set_keepalive(int fd)
{
	const int optval = 1;
	int r = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
		&optval, sizeof optval);
	(void) r;
	assert(r != -1);
}
	
void disable_nagle(int fd)
{
	const int optval = 1;
	int r = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
		&optval, sizeof optval);
	(void) r;
	assert(r != -1);
}

void set_reuseaddress(int fd)
{
	const int optval = 1;
	int r = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
		&optval, sizeof optval);
	(void) r;
	assert(r != -1);
}

void await_input(int fd)
{
	struct pollfd pfds[1];

	pfds[0].fd = fd;
	pfds[0].events = POLLIN;
	pfds[0].revents = 0;

	int r;
	do {
		r = poll(pfds, 1, -1);
	} while (r == -1 && errno == EINTR);

	assert(r == 1);
}

void await_output(int fd)
{
	struct pollfd pfds[1];

	pfds[0].fd = fd;
	pfds[0].events = POLLOUT;
	pfds[0].revents = 0;

	int r;
	do {
		r = poll(pfds, 1, -1);
	} while (r == -1 && errno == EINTR);

	assert(r == 1);
}

void get_local_ip_address(char buf[ip_buf_size], int fd)
{
	address_storage storage;
	socklen_t length = sizeof storage;

	int r = getsockname(fd, &storage.sa, &length);
	(void) r;
	assert(r == 0);
	get_ip_address(buf, &storage, length);
}

int get_local_port_number(int fd)
{
	address_storage storage;
	socklen_t length = sizeof storage;

	int r = getsockname(fd, &storage.sa, &length);
	(void) r;
	assert(r == 0);
	return get_port_number(&storage, length);
}

void get_remote_ip_address(char buf[ip_buf_size], int fd)
{
	address_storage storage;
	socklen_t length = sizeof storage;

	int r = getpeername(fd, &storage.sa, &length);
	(void) r;
	assert(r == 0);
	get_ip_address(buf, &storage, length);
}

int get_remote_port_number(int fd)
{
	address_storage storage;
	socklen_t length = sizeof storage;

	int r = getpeername(fd, &storage.sa, &length);
	(void) r;
	assert(r == 0);
	return get_port_number(&storage, length);
}

