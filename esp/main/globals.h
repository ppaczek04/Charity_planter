#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>

#define TOPIC_PREFIX "user1/esp32_test"

#define WATER_TOPIC "user1/esp32_test/water"

#define TOPIC_SOIL        TOPIC_PREFIX "/soil"
#define TOPIC_TEMP        TOPIC_PREFIX "/temperature"
#define TOPIC_PRESS       TOPIC_PREFIX "/pressure"
#define TOPIC_COIN        TOPIC_PREFIX "/coin_inserted"

#define NVS_NS_SOIL       "soil_buf"
#define NVS_NS_TEMP       "temp_buf"
#define NVS_NS_PRESS      "press_buf"
#define NVS_NS_COIN       "coin_buf"

enum Parameter {
    SOIL_HUMIDITY,
    TEMPERATURE,
    PRESSURE,
    COIN_INSERTED
};

#endif