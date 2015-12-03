#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_store.h"
#include "dispatcher.h"
#include "lprintf.h"
#include "stop_handler.h"

static const char default_ip[] = "127.0.0.1";
enum { default_port = 0 };

static const char *ip = default_ip;
static int port = default_port;

static int usage(const char *argv0)
{
	fprintf(stderr, "usage: %s [<option>...]\n", argv0);
	fprintf(stderr, "options are:\n");
	fprintf(stderr,
		"  --ip <address>      sets ip address (default: %s)\n",
			default_ip);
	fprintf(stderr,
		"  --loglevel <level>  sets log level\n");
	fprintf(stderr,
		"  --port <number>     sets port number (default: %d)\n",
			default_port);

	return 1;
}

static int parse_options(int argc, char *argv[])
{
	int i;

	for (i = 1; i != argc && *argv[i] == '-'; ++i) {

		if (strcmp(argv[i], "--ip") == 0) {
			
			if (++i == argc) {
				return -1;
			}

			ip = argv[i];

		} else if (strcmp(argv[i], "--loglevel") == 0) {

			if (++i == argc) {
				return -1;
			}
			set_loglevel(atoi(argv[i]));

		} else if (strcmp(argv[i], "--port") == 0) {

			if (++i == argc) {
				return -1;
			}
			port = atoi(argv[i]);

		} else {

			return -1;
		}
	}

	return i;
}	
			
int main(int argc, char *argv[])
{
	if (parse_options(argc, argv) != argc) {
		return usage(argv[0]);
	}

	lprintf(info, "%s: initializing\n", argv[0]);

	return_code rc;

	dispatcher *disp;
	rc = dispatcher_create(&disp);
	if (rc != ok) {
		lprintf(fatal, "%s: can't create dispatcher: %s\n",
			argv[0], return_code_string(rc));
		return 1;
	}

	stop_handler *sh;
	rc = stop_handler_create(&sh, disp);
	if (rc != ok) {
		lprintf(fatal, "%s: can't create stop handler: %s\n",
			argv[0], return_code_string(rc));
		dispatcher_destroy(disp);
		return 1;
	}
	
	data_store *store;
	rc = data_store_create(&store, disp, ip, port);
	if (rc != ok) {
		lprintf(fatal, "%s: can't create data store: %s\n",
			argv[0], return_code_string(rc));
		stop_handler_destroy(sh);
		dispatcher_destroy(disp);
		return 1;
	}

	lprintf(info, "%s: running\n", argv[0]);

	dispatcher_run(disp);

	lprintf(info, "%s: cleaning up\n", argv[0]);

	data_store_destroy(store);
	stop_handler_destroy(sh);
	dispatcher_destroy(disp);

	lprintf(info, "%s: done\n", argv[0]);

	return 0;
}
