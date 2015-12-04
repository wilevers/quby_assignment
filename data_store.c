#include <assert.h>
#include <stdlib.h>

#include "data_session.h"
#include "data_store.h"
#include "lprintf.h"

struct data_store {
	dispatcher *disp;
	acceptor *acc;
	io_slot *acc_slot;
	map *data;
	data_session **sessions;
	int n_sessions;
	int n_sessions_alloc;
};

static return_code on_accept(void *user_data)
{
	data_store *store = user_data;

	if (store->n_sessions == store->n_sessions_alloc) {
		
		int new_alloc = store->n_sessions_alloc +
			store->n_sessions_alloc / 2 + 1;
		data_session **new_sessions = store->sessions == NULL ?
			malloc(sizeof *new_sessions * new_alloc) :
			realloc(store->sessions,
				sizeof *new_sessions * new_alloc);

		if (new_sessions == NULL) {
			return out_of_memory;
		}

		store->n_sessions_alloc = new_alloc;
		store->sessions = new_sessions;
	}
			
	return_code rc = data_session_create(
		&store->sessions[store->n_sessions],
		store->disp, store, store->acc);

	switch (rc) {
	case ok :
		++store->n_sessions;
		break;
	case would_block :
		/* just my luck */
		break;
	default :
		return rc;
		break;
	}
		
	acceptor_activate_io_slot(store->acc, store->disp,
		store->acc_slot, &on_accept, store);

	return ok;
}

return_code data_store_create(data_store **result,
	dispatcher *disp, const char *ip_address, int port)
{
	data_store *store = malloc(sizeof *store);
	if (store == NULL) {
		return out_of_memory;
	}

	store->disp = disp;

	return_code rc = acceptor_create(&store->acc, ip_address, port);
	if (rc != ok) {
		free(store);
		return rc;
	}

	rc = dispatcher_create_io_slot(store->disp, &store->acc_slot);
	if (rc != ok) {
		acceptor_destroy(store->acc);
		free(store);
		return rc;
	}

	rc = map_create(&store->data);
	if (rc != ok) {
		dispatcher_destroy_io_slot(store->disp, store->acc_slot);
		acceptor_destroy(store->acc);
		free(store);
		return rc;
	}

	store->sessions = NULL;
	store->n_sessions = 0;
	store->n_sessions_alloc = 0;

	acceptor_activate_io_slot(store->acc, store->disp,
		store->acc_slot, &on_accept, store);

	lprintf(info, "data store listening at %s port %d\n",
		acceptor_ip(store->acc), acceptor_port(store->acc));

	*result = store;
	return ok;
}

const char *data_store_ip(const data_store *store)
{
	return acceptor_ip(store->acc);
}

int data_store_port(const data_store *store)
{
	return acceptor_port(store->acc);
}

const map *data_store_data(const data_store *store)
{
	return store->data;
}

return_code data_store_update(data_store *store, const map *src)
{
	int n_src_keys = map_get_n_keys(src);
	int i;
	for (i = 0; i != n_src_keys; ++i) {

		return_code rc = map_set_value(store->data,
			map_get_key(src, i), 
			map_get_value(src, i)
		);
		if (rc != ok) {
			return rc;
		}
	}

	return ok;
}
		
void data_store_stop_session(data_store *store, data_session *sess)
{
	int i;
	for (i = 0; i != store->n_sessions; ++i) {
		if (store->sessions[i] == sess) {
			break;
		}
	}

	assert(i != store->n_sessions);
	data_session_destroy(sess);

	--store->n_sessions;
	for (; i != store->n_sessions; ++i) {
		store->sessions[i] = store->sessions[i + 1];
	}
}
		
void data_store_destroy(data_store *store)
{
	lprintf(info, "closing data store at %s port %d\n",
		acceptor_ip(store->acc), acceptor_port(store->acc));

	int i;
	for (i = 0; i != store->n_sessions; ++i) {
		data_session_destroy(store->sessions[i]);
	}
	free(store->sessions);	

	map_destroy(store->data);
	dispatcher_destroy_io_slot(store->disp, store->acc_slot);
	acceptor_destroy(store->acc);
	free(store);
}
