#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "mqtt_client.h"

void mqtt_manager_init(void);
void mqtt_send_sensor_data(int soil, float temp, float press);
void mqtt_send_coin_event(void);

#endif