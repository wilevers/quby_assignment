#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_source.h"
#include "dispatcher.h"
#include "lprintf.h"
#include "stop_handler.h"

static int usage(const char *argv0)
{
	fprintf(stderr, "usage: %s [<option>...]"
		" <target host> <target port\n", argv0);
	fprintf(stderr, "options are:\n");
	fprintf(stderr, "  --loglevel <level>  sets log level\n");
	return 1;
}

static int parse_options(int argc, char *argv[])
{
	int i;
	for (i = 1; i != argc && *argv[i] == '-'; ++i) {
	
		if (strcmp(argv[i], "--loglevel") == 0) {

			if (++i == argc) {
				return -1;
			}
			set_loglevel(atoi(argv[i]));
		
		} else {
			return -1;
		}
	}

	return i;
}

int main(int argc, char *argv[])
{
	int i = parse_options(argc, argv);
	if (i == -1) {
		return usage(argv[0]);
	}

	if (i == argc) {
		return usage(argv[0]);
	}
	const char *target_host = argv[i];
	++i;

	if (i == argc) { 
		return usage(argv[0]);
	}
	int target_port = atoi(argv[i]);
	++i;

	if (i != argc) {
		return usage(argv[0]);
	}

	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	if (rc != ok) {
		fprintf(stderr, "%s: %s\n", argv[0], return_code_string(rc));
		return 1;
	}

	stop_handler *sh;
	rc = stop_handler_create(&sh, disp);
	if (rc != ok) {
		fprintf(stderr, "%s: %s\n", argv[0], return_code_string(rc));
		dispatcher_destroy(disp);
		return 1;
	}

	data_source *src;
	rc = data_source_create(&src, disp, target_host, target_port);
	if (rc != ok) {
		fprintf(stderr, "%s: %s\n", argv[0], return_code_string(rc));
		stop_handler_destroy(sh);
		dispatcher_destroy(disp);
		return 1;
	}
	
	lprintf(info, "%s: running\n", argv[0]);
	dispatcher_run(disp);
	lprintf(info, "%s: shutting down\n", argv[0]);

	data_source_destroy(src);
	stop_handler_destroy(sh);
	dispatcher_destroy(disp);

	return 0;
}
