#include <stdlib.h>

#include "lprintf.h"
#include "light_sensor.h"

typedef enum { dark, bright } condition;

struct light_sensor {
	dispatcher *disp;
	data_source *src;
	alarm_slot *alarm;
	condition cond;
};

static void on_alarm(void *user_data)
{
	light_sensor *sensor = user_data;
	
	switch (sensor->cond) {
	case dark :
		sensor->cond = bright;
		break;
	case bright :
		sensor->cond = dark;
		break;
	};

	const char *report = sensor->cond == bright ? "bright" : "dark";

	return_code rc = data_source_set_string_value(sensor->src,
		"lightCondition", report);
	if (rc != ok) {
		lprintf(fatal, "light sensor: %s\n",
			return_code_string(rc));
		dispatcher_stop(sensor->disp);
		return;
	}

	lprintf(info, "light condition reported\n");

	dispatcher_activate_alarm_slot(sensor->disp, sensor->alarm,
		((rand() % 15) + 1) * 1000, &on_alarm, sensor);
}

return_code light_sensor_create(light_sensor **result,
	dispatcher *disp, data_source *src)
{
	light_sensor *sensor = malloc(sizeof *sensor);
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

	sensor->cond = dark;

	dispatcher_activate_alarm_slot(sensor->disp, sensor->alarm,
		((rand() % 15) + 1) * 1000, &on_alarm, sensor);

	lprintf(info, "light sensor created\n");

	*result = sensor;
	return ok;
}

void light_sensor_destroy(light_sensor *sensor)
{
	lprintf(info, "destroying light sensor\n");

	dispatcher_destroy_alarm_slot(sensor->disp, sensor->alarm);
	free(sensor);
}
	