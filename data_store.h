#ifndef DATA_STORE_H
#define DATA_STORE_H

#include "dispatcher.h"
#include "map.h"
#include "return_code.h"

typedef struct data_store data_store;
typedef struct data_session data_session;

return_code data_store_create(data_store **result,
	dispatcher *disp, const char *ip_address, int port);

const char *data_store_ip(const data_store *store);
int data_store_port(const data_store *store);

const map *data_store_data(const data_store *store);
return_code data_store_update(data_store *store, const map *src);
void data_store_disconnect(data_store *store, data_session *sess);

void data_store_destroy(data_store *store);

#endif
