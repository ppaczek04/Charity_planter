#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include "esp_err.h"

void ble_config_start(bool mode_permanent);
void ble_config_stop(void);

#endif