#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

#include <stdint.h>

void soil_sensor_init(void);
int soil_sensor_read_raw(void);
int soil_sensor_get_percentage(void);

#endif