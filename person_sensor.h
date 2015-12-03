#ifndef PERSON_SENSOR_H
#define PERSON_SENSOR_H

#include "data_source.h"
#include "dispatcher.h"
#include "return_code.h"

typedef struct person_sensor person_sensor;

return_code person_sensor_create(person_sensor **result,
	dispatcher *disp, data_source *src);

void person_sensor_destroy(person_sensor *sensor);

#endif