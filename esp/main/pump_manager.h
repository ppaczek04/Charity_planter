#ifndef PUMP_MANAGER_H
#define PUMP_MANAGER_H

#include <stdbool.h>

void pump_manager_init(void);
void pump_turn_on(void);  // Włącz pompkę
void pump_turn_off(void); // Wyłącz pompkę

#endif