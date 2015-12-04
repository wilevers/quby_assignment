#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "return_code.h"

typedef enum {
	input,
	output
} io_mode;

typedef struct dispatcher dispatcher;
typedef struct io_slot io_slot;
typedef struct alarm_slot alarm_slot;

return_code dispatcher_create(dispatcher **result);

return_code dispatcher_create_io_slot(dispatcher *disp, io_slot **result);

void dispatcher_activate_io_slot(dispatcher *disp,
	io_slot *slot, int fd, io_mode mode, 
	return_code (*callback)(void *), void *callback_arg);

void dispatcher_deactivate_io_slot(dispatcher *disp, io_slot *slot);

void dispatcher_destroy_io_slot(dispatcher *disp, io_slot *slot);

return_code dispatcher_create_alarm_slot(dispatcher *disp,
	alarm_slot **result);

void dispatcher_activate_alarm_slot(dispatcher *disp,
	alarm_slot *slot, unsigned int msecs,
	return_code (*callback)(void *), void *callback_arg);

void dispatcher_deactivate_alarm_slot(dispatcher *disp, alarm_slot *slot);

void dispatcher_destroy_alarm_slot(dispatcher *disp, alarm_slot *slot);

return_code dispatcher_run(dispatcher *disp);

void dispatcher_stop(dispatcher *disp);

void dispatcher_destroy(dispatcher *disp);

#endif
