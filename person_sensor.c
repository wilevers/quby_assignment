#include <stdlib.h>

#include "lprintf.h"
#include "person_sensor.h"

struct person_sensor {
	dispatcher *disp;
	data_source *src;
	alarm_slot *alarm;
	int count;
};

static void on_alarm(void *user_data)
{
	person_sensor *sensor = user_data;
	++sensor->count;
	
	return_code rc = data_source_set_integer_value(sensor->src,
		"personsPassed", sensor->count);
	if (rc != ok) {
		lprintf(fatal, "person sensor: %s\n",
			return_code_string(rc));
		dispatcher_stop(sensor->disp);
		return;
	}

	lprintf(info, "person reported\n");

	dispatcher_activate_alarm_slot(sensor->disp, sensor->alarm,
		((rand() % 10) + 1) * 1000, &on_alarm, sensor);
}

return_code person_sensor_create(person_sensor **result,
	dispatcher *disp, data_source *src)
{
	person_sensor *sensor = malloc(sizeof *sensor);
	if (sensor == NULL) {
		return out_of_memory;
	}

	sensor->disp = disp;
	sensor->src = src;
	
	return_code rc = dispatcher_create_alarm_slot(disp, &sensor->alarm);
	if (rc != ok) {
		free(sensor);
		return rc;
	}

	sensor->count = 0;

	dispatcher_activate_alarm_slot(sensor->disp, sensor->alarm,
		((rand() % 10) + 1) * 1000, &on_alarm, sensor);

	lprintf(info, "person sensor created\n");

	*result = sensor;
	return ok;
}

void person_sensor_destroy(person_sensor *sensor)
{
	lprintf(info, "destroying person sensor\n");

	dispatcher_destroy_alarm_slot(sensor->disp, sensor->alarm);
	free(sensor);
}
	