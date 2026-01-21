#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>

extern uint32_t g_measurement_interval_ms;
#define DEFAULT_INTERVAL_MS 5000

#define TOPIC_SUFFIX_CONFIG       "config"
#define TOPIC_SUFFIX_WATER        "water"
#define TOPIC_SUFFIX_SOIL         "soil"
#define TOPIC_SUFFIX_TEMP         "temperature"
#define TOPIC_SUFFIX_PRESS        "pressure"
#define TOPIC_SUFFIX_COIN         "coin_inserted"
#define TOPIC_SUFFIX_ACK          "water/ack"

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