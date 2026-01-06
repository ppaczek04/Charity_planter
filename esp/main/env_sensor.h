#ifndef ENV_SENSOR_H
#define ENV_SENSOR_H

#include <stdbool.h>

void env_sensor_init(void);
bool env_sensor_read(float *temp, float *pressure);

#endif