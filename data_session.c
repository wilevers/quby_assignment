#include <stdlib.h>
#include "connection.h"
#include "data_session.h"
#include "lprintf.h"

struct data_session {
	dispatcher *disp;
	data_store *store;
	connection *conn;
};

return_code data_session_create(data_session **result,
	dispatcher *disp, data_store *store, acceptor *acc)
{
	data_session *sess = malloc(sizeof *sess);
	if (sess == NULL) {
		return out_of_memory;
	}

	sess->disp = disp;
	sess->store = store;

	return_code rc = acceptor_accept_nonblocking(acc, &sess->conn);
	if (rc != ok) {
		free(sess);
		return rc;
	}

	lprintf(info, "new session %s port %d <-> %s port %d\n",
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
	lprintf(info, "closing session %s port %d <-> %s port %d\n",
		connection_local_ip(sess->conn),
		connection_local_port(sess->conn),
		connection_remote_ip(sess->conn),
		connection_remote_port(sess->conn)
	);
		
	connection_destroy(sess->conn);
	free(sess);
}
