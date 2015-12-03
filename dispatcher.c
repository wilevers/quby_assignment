#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#include <poll.h>
#include <sys/time.h>

#include "dispatcher.h"

typedef struct {
	time_t secs;
	unsigned int msecs;
} timepoint;

struct io_slot {
	int fd;
	io_mode mode;
	void (*callback)(void *);
	void *user_data;
	io_slot *prev;
	io_slot *next;
};

struct alarm_slot {
	timepoint tp;
	void (*callback)(void *);
	void *user_data;
	alarm_slot *prev;
	alarm_slot *next;
};

struct dispatcher {

	struct pollfd *pfds;
	int n_pfds;
	int n_ios;

	io_slot *first_io;
	io_slot *last_io;
	io_slot *first_active_io;
	io_slot *first_inactive_io;

	alarm_slot *first_alarm;
	alarm_slot *last_alarm;
	alarm_slot *first_active_alarm;
	alarm_slot *first_inactive_alarm;

	int stopping;
};

static void timepoint_zero(timepoint *tp)
{
	tp->secs = 0;
	tp->msecs = 0;
}

static void timepoint_now(timepoint *tp)
{
	struct timeval tv;

	int r = gettimeofday(&tv, NULL);
	(void) r;
	assert(r == 0);

	tp->secs = tv.tv_sec;
	tp->msecs = tv.tv_usec / 1000;
}

static void timepoint_add(timepoint *tp, unsigned int msecs)
{
	tp->msecs += msecs % 1000;
	if (tp->msecs >= 1000) {
		tp->secs += 1;
		tp->msecs -= 1000;
	}
	tp->secs += msecs / 1000;
}
		
static int timepoint_less(const timepoint *lhs, const timepoint *rhs)
{
	return lhs->secs < rhs->secs ? 1 :
		rhs->secs < lhs->secs ? 0 :
		lhs->msecs < rhs->msecs;
}

static unsigned int timepoint_subtract(const timepoint *lhs,
	const timepoint *rhs)
{
	assert(! timepoint_less(lhs, rhs));
	
	unsigned int secs = lhs->secs - rhs->secs;
	unsigned int msecs = lhs->msecs;
	if (msecs < rhs->msecs) {
		secs -= 1;
		msecs += 1000;
	}
	msecs -= rhs->msecs;

	return 1000 * secs + msecs;
}			

static void insert_io_slot(io_slot *slot, dispatcher *disp, io_slot *before)
{
	if (before == NULL) {
		slot->prev = disp->last_io;
	} else {
		slot->prev = before->prev;
	}
	slot->next = before;

	if (slot->prev == NULL) {
		disp->first_io = slot;
	} else {
		slot->prev->next = slot;
	}

	if (slot->next == NULL) {
		disp->last_io = slot;
	} else {
		slot->next->prev = slot;
	}
}

static void remove_io_slot(dispatcher *disp, io_slot *slot)
{
	if (slot == disp->first_active_io) {
		disp->first_active_io = slot->next;
	}
	if (slot == disp->first_inactive_io) {
		disp->first_inactive_io = slot->next;
	}

	if (slot->prev == NULL) {
		disp->first_io = slot->next;
	} else {
		slot->prev->next = slot->next;
	}

	if (slot->next == NULL) {
		disp->last_io = slot->prev;
	} else {
		slot->next->prev = slot->prev;
	}
}

static void insert_alarm_slot(
	alarm_slot *slot, dispatcher *disp, alarm_slot *before)
{
	if (before == NULL) {
		slot->prev = disp->last_alarm;
	} else {
		slot->prev = before->prev;
	}
	slot->next = before;

	if (slot->prev == NULL) {
		disp->first_alarm = slot;
	} else {
		slot->prev->next = slot;
	}

	if (slot->next == NULL) {
		disp->last_alarm = slot;
	} else {
		slot->next->prev = slot;
	}
}

static void remove_alarm_slot(dispatcher *disp, alarm_slot *slot)
{
	if (slot == disp->first_active_alarm) {
		disp->first_active_alarm = slot->next;
	}
	if (slot == disp->first_inactive_alarm) {
		disp->first_inactive_alarm = slot->next;
	}

	if (slot->prev == NULL) {
		disp->first_alarm = slot->next;
	} else {
		slot->prev->next = slot->next;
	}

	if (slot->next == NULL) {
		disp->last_alarm = slot->prev;
	} else {
		slot->next->prev = slot->prev;
	}
}
	
static void await_events(dispatcher *disp)
{
	timepoint now;
	timepoint_now(&now);

	timepoint deadline = now;
	timepoint_add(&deadline, 30000);

	if (disp->first_active_alarm != disp->first_inactive_alarm &&
		timepoint_less(&disp->first_active_alarm->tp, &deadline)) {

		if (timepoint_less(&disp->first_active_alarm->tp, &now)) {
			deadline = now;
		} else {
			deadline = disp->first_active_alarm->tp; 
		}
	}

	unsigned int timeout = timepoint_subtract(&deadline, &now);

	int n_pfds = 0;
	io_slot *io;
	for (io = disp->first_active_io; io != disp->first_inactive_io;
		io = io->next) {

		assert(n_pfds != disp->n_pfds);

		disp->pfds[n_pfds].fd = io->fd;
		disp->pfds[n_pfds].events =
			io->mode == input ? POLLIN : POLLOUT;
		disp->pfds[n_pfds].revents = 0;
	
		++n_pfds;
	}

	int r = poll(disp->pfds, n_pfds, timeout);
	if (r < 0) {
		assert(errno == EINTR);
		r = 0;
	}

	int idx = 0;
	io = disp->first_active_io;
	while (r != 0) {

		assert(io != disp->first_inactive_io);
		assert(idx != n_pfds);

		io_slot *next = io->next;
		if (disp->pfds[idx].revents != 0) {
			remove_io_slot(disp, io);
			insert_io_slot(io, disp, disp->first_active_io);
			--r;
		}

		io = next;
		++idx;
	}

	timepoint_now(&now);

	while (disp->first_active_alarm != disp->first_inactive_alarm &&
		! timepoint_less(&now, &disp->first_active_alarm->tp)) {
		disp->first_active_alarm = disp->first_active_alarm->next;
	}
}

return_code dispatcher_create(dispatcher **result)
{
	dispatcher *disp = malloc(sizeof *disp);
	if (disp == NULL) {
		return out_of_memory;
	}

	disp->pfds = NULL;
	disp->n_pfds = 0;
	disp->n_ios = 0;

	disp->first_io = NULL;
	disp->last_io = NULL;
	disp->first_active_io = NULL;
	disp->first_inactive_io = NULL;

	disp->first_alarm = NULL;
	disp->last_alarm = NULL;
	disp->first_active_alarm = NULL;
	disp->first_inactive_alarm = NULL;

	disp->stopping = 0;

	*result = disp;
	return ok;
}

return_code dispatcher_create_io_slot(dispatcher *disp, io_slot **result)
{
	if (disp->n_ios == disp->n_pfds) {

		int new_n_pfds = disp->n_pfds + disp->n_pfds / 2 + 1;
		struct pollfd *new_pfds = disp->pfds == NULL ?
			malloc(sizeof *new_pfds * new_n_pfds) :
			realloc(disp->pfds, sizeof *new_pfds * new_n_pfds);

		if (new_pfds == NULL) {
			return out_of_memory;
		}

		disp->pfds = new_pfds;
		disp->n_pfds = new_n_pfds;
	}
		
 	io_slot *slot = malloc(sizeof *slot);
	if (slot == NULL) {
		return out_of_memory;
	}

	slot->fd = -1;
	slot->mode = input;
	slot->callback = NULL;
	slot->user_data = NULL;

	insert_io_slot(slot, disp, NULL);
	
	if (disp->first_inactive_io == NULL) {
		if (disp->first_active_io == NULL) {
			disp->first_active_io = slot;
		}
		disp->first_inactive_io = slot;
	}

	++disp->n_ios;

	*result = slot;
	return ok;
}

void dispatcher_activate_io_slot(dispatcher *disp, io_slot *slot,
	int fd, io_mode mode, void (*callback)(void *), void *user_data)
{
	remove_io_slot(disp, slot);

	slot->fd = fd;
	slot->mode = mode;
	slot->callback = callback;
	slot->user_data = user_data;

	insert_io_slot(slot, disp, disp->first_inactive_io);

	if (disp->first_active_io == disp->first_inactive_io) {
		disp->first_active_io = slot;
	}
}

void dispatcher_deactivate_io_slot(dispatcher *disp, io_slot *slot)
{
	remove_io_slot(disp, slot);
	insert_io_slot(slot, disp, NULL);

	if (disp->first_inactive_io == NULL) {
		if (disp->first_active_io == NULL) {
			disp->first_active_io = slot;
		}
		disp->first_inactive_io = slot;
	}
}

void dispatcher_destroy_io_slot(dispatcher *disp, io_slot *slot)
{
	assert(disp->n_ios > 0);
	--disp->n_ios;

	remove_io_slot(disp, slot);
	free(slot);
}

return_code dispatcher_create_alarm_slot(dispatcher *disp,
	alarm_slot **result)
{
	alarm_slot *slot = malloc(sizeof *slot);
	if (slot == NULL) {
		return out_of_memory;
	}

	timepoint_zero(&slot->tp);
	slot->callback = NULL;
	slot->user_data = NULL;

	insert_alarm_slot(slot, disp, NULL);

	if (disp->first_inactive_alarm == NULL) {
		if (disp->first_active_alarm == NULL) {
			disp->first_active_alarm = slot;
		}
		disp->first_inactive_alarm = slot;
	}

	*result = slot;
	return ok;
}

void dispatcher_activate_alarm_slot(dispatcher *disp,
	alarm_slot *slot, unsigned int msecs,
	void (*callback)(void *), void *user_data)
{
	remove_alarm_slot(disp, slot);

	timepoint_now(&slot->tp);
	timepoint_add(&slot->tp, msecs);
	slot->callback = callback;
	slot->user_data = user_data;

	alarm_slot *next;
	for (next = disp->first_active_alarm;
		next != disp->first_inactive_alarm;
		next = next->next) {
		if (timepoint_less(&slot->tp, &next->tp)) {
			break;
		}
	}

	insert_alarm_slot(slot, disp, next);
	if (disp->first_active_alarm == next) {
		disp->first_active_alarm = slot;
	}
}

void dispatcher_deactivate_alarm_slot(dispatcher *disp, alarm_slot *slot)
{
	remove_alarm_slot(disp, slot);
	insert_alarm_slot(slot, disp, NULL);

	if (disp->first_inactive_alarm == NULL) {
		if (disp->first_active_alarm == NULL) {
			disp->first_active_alarm = slot;
		}
		disp->first_inactive_alarm = slot;
	}
}
	
void dispatcher_destroy_alarm_slot(dispatcher *disp, alarm_slot *slot)
{
	remove_alarm_slot(disp, slot);
	free(slot);
}

void dispatcher_run(dispatcher *disp)
{
	while (! disp->stopping &&
		(disp->first_io != disp->first_inactive_io ||
		disp->first_alarm != disp->first_inactive_alarm)) {
		
		io_slot *io;
		alarm_slot *alarm;

		if ((io = disp->first_io) != disp->first_active_io) {

			dispatcher_deactivate_io_slot(disp, io);
			(*io->callback)(io->user_data);

		} else if ((alarm = disp->first_alarm) != 
			disp->first_active_alarm) {

			dispatcher_deactivate_alarm_slot(disp, alarm);
			(*alarm->callback)(alarm->user_data);

		} else {

			await_events(disp);
		}
	}

	disp->stopping = 0;
}

void dispatcher_stop(dispatcher *disp)
{
	disp->stopping = 1;
}

void dispatcher_destroy(dispatcher *disp)
{
	assert(disp->first_alarm == NULL);
	assert(disp->first_io == NULL);
	assert(disp->n_ios == 0);

	free(disp->pfds);

	free(disp);
}
