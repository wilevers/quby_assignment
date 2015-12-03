#ifndef DATA_SESSION_H
#define DATA_SESSION_H

#include "acceptor.h"
#include "data_store.h"

return_code data_session_create(data_session **result,
	dispatcher *disp, data_store *store, acceptor *acc);

void data_session_destroy(data_session *sess);

#endif
