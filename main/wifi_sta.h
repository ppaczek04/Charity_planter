#ifndef WIFI_STA_H
#define WIFI_STA_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

extern EventGroupHandle_t s_wifi_event_group;

extern const EventBits_t WIFI_CONNECTED_BIT;
extern const EventBits_t WIFI_CONNECTING_BIT;

void wifi_init_sta(void);
void blink_led_task(void *pvParameter);

#endif