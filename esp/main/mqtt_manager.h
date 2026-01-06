#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "mqtt_client.h"

typedef void (*water_command_callback_t)(float duration_sec);

void mqtt_manager_init(void);
void mqtt_send_sensor_data(int soil, float temp, float press);
void mqtt_send_coin_event(void);
void set_water_command_callback(water_command_callback_t callback);

#endif