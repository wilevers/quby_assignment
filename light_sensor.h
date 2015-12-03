#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "data_source.h"
#include "dispatcher.h"
#include "return_code.h"

typedef struct light_sensor light_sensor;

return_code light_sensor_create(light_sensor **result,
	dispatcher *disp, data_source *src);

void light_sensor_destroy(light_sensor *sensor);

#endif