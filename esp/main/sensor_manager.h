#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdbool.h>

// Funkcja inicjalizująca wszystko (I2C, GPIO, ADC)
void sensor_manager_init(void);

// Funkcje odczytu
int sensor_manager_read_soil_percent(void);
float sensor_manager_read_temperature(void);
float sensor_manager_read_pressure(void);

// Funkcja sprawdzająca wrzutnik monet
bool sensor_manager_is_coin_inserted(void);

#endif