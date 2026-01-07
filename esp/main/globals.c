#include "globals.h"

volatile bool is_wifi_connected = false;

char user_mac[18] = "00:00:00:00:00:00";

char device_mac[18] = "00:00:00:00:00:00";

const char *MQTT_TAG = "MQTT";
const char *WIFI_TAG = "WIFI";