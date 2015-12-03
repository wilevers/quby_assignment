#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "acceptor.h"
#include "connection.h"
#include "lprintf.h"
#include "stop_handler.h"

static const int signals[] = { SIGINT, SIGTERM };
enum { n_signals = sizeof signals / sizeof signals[0] };

struct stop_handler {
	dispatcher *disp;
	connection *sender;
	connection *receiver;
	io_slot *receiver_slot;
	struct sigaction saved_actions[n_signals];
};

static stop_handler *current_stop_handler = NULL;

static void signal_handler(int sig)
{
	char c = sig;
	int saved_errno = errno;

	int bytes_sent_ignored;
	connection_send_nonblocking(current_stop_handler->sender, 
		&bytes_sent_ignored, &c, sizeof c);

	errno = saved_errno;
}

static void input_handler(void *user_data)
{
	stop_handler *sh = user_data;

	char c;
	int bytes_received = 0;
	return_code rc = connection_receive_nonblocking(
		sh->receiver, &bytes_received, &c, sizeof c);

	if (rc == ok && bytes_received == sizeof c) {

		int sig = c;
		lprintf(info, "%s: detected signal %d: stopping dispatcher\n",
			__FILE__, sig);
		dispatcher_stop(sh->disp);
	}

	connection_activate_io_slot(sh->receiver, sh->disp,
		sh->receiver_slot, input, &input_handler, sh);
}

return_code stop_handler_create(stop_handler **result, dispatcher *disp)
{
	stop_handler *sh;
	sh = malloc(sizeof *sh);
	if (sh == NULL) {
		return out_of_memory;
	}

	sh->disp = disp;

	acceptor *acc;
	return_code rc = acceptor_create(&acc, "127.0.0.1", 0);
	if (rc != ok) {
		free(sh);
		return rc;
	}

	rc = connection_create(&sh->sender, "127.0.0.1", acceptor_port(acc));
	if (rc != ok) {
		acceptor_destroy(acc);
		free(sh);
		return rc;
	}

	rc = acceptor_accept_nonblocking(acc, &sh->receiver);
	if (rc != ok) {
		connection_destroy(sh->sender);
		acceptor_destroy(acc);
		free(sh);
		return rc;
	}

	acceptor_destroy(acc);

	rc = dispatcher_create_io_slot(disp, &sh->receiver_slot);
	if (rc != ok) {
		connection_destroy(sh->receiver);
		connection_destroy(sh->sender);
		free(sh);
		return rc;
	}

	assert(current_stop_handler == NULL);
	current_stop_handler = sh;

	struct sigaction new_action;
	memset(&new_action, '\0', sizeof new_action);
	new_action.sa_handler = &signal_handler;
	new_action.sa_flags = SA_RESTART;

	int i;
	for (i = 0; i != n_signals; ++i) {
		int r = sigaction(signals[i],
			&new_action, &sh->saved_actions[i]);
		(void) r;
		assert(r == 0);
	}

	connection_activate_io_slot(sh->receiver, sh->disp,
		sh->receiver_slot, input, input_handler, sh);		
		
	*result = sh;
	return ok;
}

void stop_handler_destroy(stop_handler *sh)
{
	dispatcher_deactivate_io_slot(sh->disp, sh->receiver_slot);

	int i;
	for (i = 0; i != n_signals; ++i) {
		sigaction(signals[i], &sh->saved_actions[i], NULL);
	}
	
	assert(current_stop_handler == sh);
	current_stop_handler = NULL;
	
	dispatcher_destroy_io_slot(sh->disp, sh->receiver_slot);
	connection_destroy(sh->receiver);
	connection_destroy(sh->sender);
	free(sh);
}
