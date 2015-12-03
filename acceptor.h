#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "connection.h"
#include "dispatcher.h"
#include "return_code.h"

typedef struct acceptor acceptor;

return_code acceptor_create(acceptor **result,
	const char *ip_address, int port);

const char *acceptor_ip(const acceptor *acceptor);
int acceptor_port(const acceptor *acceptor);

void acceptor_activate_io_slot(acceptor *acc,
	dispatcher *disp, io_slot *slot,
	void (*callback)(void *), void *user_data);

return_code acceptor_accept_blocking(acceptor *acc, connection **result);
return_code acceptor_accept_nonblocking(acceptor *acc, connection **result);

void acceptor_destroy(acceptor *acc);

#endif
