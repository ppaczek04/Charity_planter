#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>

extern volatile bool is_wifi_connected;
extern char user_mac[18];
extern char device_mac[18];

// Tagi logów
extern const char *MQTT_TAG;

// Tematy WYSYŁANIA (Publish)
#define TEMPERATURE_TOPIC   "%s/%s/temperature"
#define PRESSURE_TOPIC      "%s/%s/pressure"
#define SOIL_HUMIDITY_TOPIC "%s/%s/soil_humidity"
#define COIN_EVENT_TOPIC    "%s/%s/coin_inserted" 

// Temat ODBIERANIA (Subscribe) - Tutaj Backend wysyła rozkaz
#define WATER_CMD_TOPIC     "%s/%s/water" 

#endif