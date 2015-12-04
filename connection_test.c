#include <string.h>

#include "acceptor.h"
#include "connection.h"

#undef NDEBUG
#include <assert.h>

static void echo_test()
{
	enum { bufsize = 1000 };

	acceptor *acc = NULL;
	return_code rc = acceptor_create(&acc, "127.0.0.1", 0);
	assert(rc == ok);
	assert(acc != NULL);
	
	connection *client = NULL;
	rc = connection_create(&client, "127.0.0.1", acceptor_port(acc));
	assert(rc == ok);
	assert(client != NULL);

	connection *server = NULL;
	rc = acceptor_accept_blocking(acc, &server);
	assert(rc == ok);
	assert(server != NULL);
	
	char outbuf[bufsize];

	int n;
	for (n = 0; n != bufsize; ++n) {
		outbuf[n] = (char) n;
	}

	n = bufsize;
	char *p = outbuf;
	while (n != 0) {
		int sent = 0;
		rc = connection_send_blocking(client, &sent, p, n);
		assert(rc == ok);
		assert(sent != 0);
		n -= sent;
		p += sent;
	}

	char srvbuf[bufsize];
	memset(srvbuf, '\0', sizeof srvbuf);
	assert(memcmp(outbuf, srvbuf, bufsize) != 0);

	n = bufsize;
	p = srvbuf;
	while (n != 0) {
		int received = 0;
		rc = connection_receive_blocking(server, &received, p, n);
		assert(rc == ok);
		assert(received != 0);
		n -= received;
		p += received;
	}
	
	assert(memcmp(outbuf, srvbuf, bufsize) == 0);

	n = bufsize;
	p = srvbuf;
	while (n != 0) {
		int sent = 0;
		rc = connection_send_blocking(server, &sent, p, n);
		assert(rc == ok);
		assert(sent != 0);
		n -= sent;
		p += sent;
	}
	
	char inbuf[bufsize];
	memset(inbuf, '\0', sizeof srvbuf);
	assert(memcmp(outbuf, inbuf, bufsize) != 0);
	
	n = bufsize;
	p = inbuf;
	while (n != 0) {
		int received = 0;
		rc = connection_receive_blocking(client, &received, p, n);
		assert(rc == ok);
		assert(received != 0);
		n -= received;
		p += received;
	}
	
	assert(memcmp(outbuf, inbuf, bufsize) == 0);

	connection_destroy(server);

	int received = 42;
	rc = connection_receive_blocking(client, &received, inbuf, bufsize);
	assert(rc == ok);
	assert(received == 0);

	connection_destroy(client);
	acceptor_destroy(acc);
}

int main()
{
	echo_test();

	return 0;
}
