#include "globals.h"

/* * DEFINICJE ZMIENNYCH GLOBALNYCH
 * Tutaj fizycznie tworzymy zmienne, które są widoczne w całym projekcie.
 */

// Status połączenia Wi-Fi (domyślnie: niepołączony)
volatile bool is_wifi_connected = false;

// Miejsce na adres MAC użytkownika (telefonu) - format XX:XX:XX:XX:XX:XX
// Wstępnie wpisujemy zera, zostaną nadpisane w main.c
char user_mac[18] = "00:00:00:00:00:00";

// Miejsce na adres MAC urządzenia (ESP32)
char device_mac[18] = "00:00:00:00:00:00";

// Tagi używane do logowania (ESP_LOGI)
// Dzięki nim w terminalu łatwo odróżnisz komunikaty MQTT od Wi-Fi
const char *MQTT_TAG = "MQTT";
const char *WIFI_TAG = "WIFI";