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
	assert(rc == 0);

	dispatcher_run(disp);

	dispatcher_destroy(disp);
}

static void test_no_active_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == 0);

	alarm_slot *alarm = NULL;
	rc = dispatcher_create_alarm_slot(disp, &alarm);
	assert(rc == 0);
	assert(alarm != NULL);

	dispatcher_run(disp);

	dispatcher_destroy_alarm_slot(disp, alarm);
	dispatcher_destroy(disp);
}

static void fail(void *user_data)
{
	assert(0);
}

static void test_deactivate_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == 0);

	alarm_slot *alarm;
	rc = dispatcher_create_alarm_slot(disp, &alarm);
	assert(rc == 0);

	dispatcher_activate_alarm_slot(disp, alarm, 0, &fail, NULL);
	dispatcher_deactivate_alarm_slot(disp, alarm);

	dispatcher_run(disp);

	dispatcher_destroy_alarm_slot(disp, alarm);
	dispatcher_destroy(disp);
}	

static void increment_int(void *user_data)
{
	int *i = user_data;
	++(*i);
}

static void test_immediate_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == 0);

	alarm_slot *alarm;
	rc = dispatcher_create_alarm_slot(disp, &alarm);
	assert(rc == 0);

	int counter = 0;
	dispatcher_activate_alarm_slot(disp, alarm, 0, &increment_int,
		&counter);

	dispatcher_run(disp);

	assert(counter == 1);
	
	dispatcher_destroy_alarm_slot(disp, alarm);
	dispatcher_destroy(disp);
}	

static void test_delayed_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == 0);

	alarm_slot *alarm;
	rc = dispatcher_create_alarm_slot(disp, &alarm);
	assert(rc == 0);

	int counter = 0;
	dispatcher_activate_alarm_slot(disp, alarm, 100,
		&increment_int, &counter);

	dispatcher_run(disp);

	assert(counter == 1);
	
	dispatcher_destroy_alarm_slot(disp, alarm);
	dispatcher_destroy(disp);
}	

static void test_stopped_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == 0);

	alarm_slot *alarm;
	rc = dispatcher_create_alarm_slot(disp, &alarm);
	assert(rc == 0);

	int counter = 0;
	dispatcher_activate_alarm_slot(disp, alarm, 0, &increment_int,
		&counter);

	dispatcher_stop(disp);
	dispatcher_run(disp);

	assert(counter == 0);
	
	dispatcher_run(disp);

	assert(counter == 1);
	
	dispatcher_destroy_alarm_slot(disp, alarm);
	dispatcher_destroy(disp);
}	

typedef struct {
	dispatcher *disp;
	alarm_slot *alarm;
	int counter;
} reactivate_data;

static void reactivate(void *user_data)
{
	reactivate_data *data = user_data;

	--data->counter;
	if (data->counter != 0) {
		dispatcher_activate_alarm_slot(
			data->disp, data->alarm, 20, &reactivate, data);
	}
}	

static void test_reactivate_alarm()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == 0);

	alarm_slot *alarm;
	rc = dispatcher_create_alarm_slot(disp, &alarm);
	assert(rc == 0);

	reactivate_data data;
	data.disp = disp;
	data.alarm = alarm;
	data.counter = 3;

	dispatcher_activate_alarm_slot(disp, alarm, 20, &reactivate, &data);

	dispatcher_run(disp);

	assert(data.counter == 0);

	dispatcher_destroy_alarm_slot(disp, alarm);
	dispatcher_destroy(disp);
}	
	
void test_multiple_alarms()
{
	dispatcher *disp;
	return_code rc = dispatcher_create(&disp);
	assert(rc == 0);

	alarm_slot *alarm1;
	rc = dispatcher_create_alarm_slot(disp, &alarm1);
	assert(rc == 0);

	alarm_slot *alarm2;
	rc = dispatcher_create_alarm_slot(disp, &alarm2);
	assert(rc == 0);

	int counter = 0;

	dispatcher_activate_alarm_slot(disp, alarm1,
		10, &increment_int, &counter);
	dispatcher_activate_alarm_slot(disp, alarm2,
		20, &increment_int, &counter);

	dispatcher_run(disp);

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

