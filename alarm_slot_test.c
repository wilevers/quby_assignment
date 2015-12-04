#include <stddef.h>
#include "dispatcher.h"

#undef NDEBUG
#include <assert.h>

static void test_create_destroy()
{
	dispatcher *disp = NULL;
	return_code rc = dispatcher_create(&disp);
	assert(rc == ok);
	assert(disp != NULL);

	dispatcher_destroy(disp);
}
	
static void test_no_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == ok);

	rc = dispatcher_run(disp);
	assert(rc == ok);

	dispatcher_destroy(disp);
}

static void test_no_active_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == ok);

	alarm_slot *alarm = NULL;
	rc = dispatcher_create_alarm_slot(disp, &alarm);
	assert(rc == ok);
	assert(alarm != NULL);

	rc = dispatcher_run(disp);
	assert(rc == ok);

	dispatcher_destroy_alarm_slot(disp, alarm);
	dispatcher_destroy(disp);
}

static return_code fail(void *user_data)
{
	assert(0);
	return ok;
}

static void test_deactivate_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == ok);

	alarm_slot *alarm;
	rc = dispatcher_create_alarm_slot(disp, &alarm);
	assert(rc == ok);

	dispatcher_activate_alarm_slot(disp, alarm, 0, &fail, NULL);
	dispatcher_deactivate_alarm_slot(disp, alarm);

	rc = dispatcher_run(disp);
	assert(rc == ok);

	dispatcher_destroy_alarm_slot(disp, alarm);
	dispatcher_destroy(disp);
}	

static return_code increment_int(void *user_data)
{
	int *i = user_data;
	++(*i);

	return ok;
}

static void test_immediate_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == ok);

	alarm_slot *alarm;
	rc = dispatcher_create_alarm_slot(disp, &alarm);
	assert(rc == ok);

	int counter = 0;
	dispatcher_activate_alarm_slot(disp, alarm, 0, &increment_int,
		&counter);

	rc = dispatcher_run(disp);
	assert(rc == ok);

	assert(counter == 1);
	
	dispatcher_destroy_alarm_slot(disp, alarm);
	dispatcher_destroy(disp);
}	

static void test_delayed_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == ok);

	alarm_slot *alarm;
	rc = dispatcher_create_alarm_slot(disp, &alarm);
	assert(rc == ok);

	int counter = 0;
	dispatcher_activate_alarm_slot(disp, alarm, 100,
		&increment_int, &counter);

	rc = dispatcher_run(disp);
	assert(rc == ok);

	assert(counter == 1);
	
	dispatcher_destroy_alarm_slot(disp, alarm);
	dispatcher_destroy(disp);
}	

static void test_stopped_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == ok);

	alarm_slot *alarm;
	rc = dispatcher_create_alarm_slot(disp, &alarm);
	assert(rc == ok);

	int counter = 0;
	dispatcher_activate_alarm_slot(disp, alarm, 0, &increment_int,
		&counter);

	dispatcher_stop(disp);

	rc = dispatcher_run(disp);
	assert(rc == ok);

	assert(counter == 0);
	
	rc = dispatcher_run(disp);
	assert(rc == ok);

	assert(counter == 1);
	
	dispatcher_destroy_alarm_slot(disp, alarm);
	dispatcher_destroy(disp);
}	

typedef struct {
	dispatcher *disp;
	alarm_slot *alarm;
	int counter;
} reactivate_data;

static return_code reactivate(void *user_data)
{
	reactivate_data *data = user_data;

	--data->counter;
	if (data->counter != 0) {
		dispatcher_activate_alarm_slot(
			data->disp, data->alarm, 20, &reactivate, data);
	}

	return ok;
}	

static void test_reactivate_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == ok);

	alarm_slot *alarm;
	rc = dispatcher_create_alarm_slot(disp, &alarm);
	assert(rc == ok);

	reactivate_data data;
	data.disp = disp;
	data.alarm = alarm;
	data.counter = 3;

	dispatcher_activate_alarm_slot(disp, alarm, 20, &reactivate, &data);

	rc = dispatcher_run(disp);
	assert(rc == ok);

	assert(data.counter == 0);

	dispatcher_destroy_alarm_slot(disp, alarm);
	dispatcher_destroy(disp);
}	
	
void test_multiple_alarms()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == ok);

	alarm_slot *alarm1;
	rc = dispatcher_create_alarm_slot(disp, &alarm1);
	assert(rc == ok);

	alarm_slot *alarm2;
	rc = dispatcher_create_alarm_slot(disp, &alarm2);
	assert(rc == ok);

	int counter = 0;

	dispatcher_activate_alarm_slot(disp, alarm1,
		10, &increment_int, &counter);
	dispatcher_activate_alarm_slot(disp, alarm2,
		20, &increment_int, &counter);

	rc = dispatcher_run(disp);
	assert(rc == ok);

	assert(counter == 2);

	dispatcher_destroy_alarm_slot(disp, alarm2);
	dispatcher_destroy_alarm_slot(disp, alarm1);
	dispatcher_destroy(disp);
}

int main()
{
	test_create_destroy();
	test_no_alarm();
	test_no_active_alarm();
	test_deactivate_alarm();
	test_immediate_alarm();
	test_stopped_alarm();
	test_delayed_alarm();
	test_reactivate_alarm();
	test_multiple_alarms();
	test_stopped_alarm();

	return 0;
}

