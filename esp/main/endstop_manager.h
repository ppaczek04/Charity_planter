#ifndef ENDSTOP_MANAGER_H
#define ENDSTOP_MANAGER_H

#include "driver/gpio.h"

#define COIN_ENDSTOP_PIN GPIO_NUM_27 

void endstop_manager_init(void);

#endif