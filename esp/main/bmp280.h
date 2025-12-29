#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c.h"


#define BMP280_I2C_ADDR_0           0x76 // Pin SDO podłączony do GND
#define BMP280_I2C_ADDR_1           0x77 // Pin SDO podłączony do VDD

#define BMP280_CHIP_ID              0x58

#define BMP280_REG_CALIB_00         0x88 // Początek danych kalibracyjnych (24 bajty)
#define BMP280_REG_ID               0xD0 // Rejestr ID układu
#define BMP280_REG_RESET            0xE0 // Rejestr resetu (Soft Reset)
#define BMP280_REG_STATUS           0xF3 // Status (czy trwa pomiar?)
#define BMP280_REG_CTRL_MEAS        0xF4 // Kontrola pomiaru (Oversampling + Mode)
#define BMP280_REG_CONFIG           0xF5 // Konfiguracja (Standby + Filtr)
#define BMP280_REG_PRESS_MSB        0xF7 // Początek danych pomiarowych (Press + Temp)


/**
 * @brief Oversampling (Nadpróbkowanie).
 * Zwiększa rozdzielczość pomiaru i redukuje szum, ale wydłuża czas pomiaru.
 * Ustawiane osobno dla temperatury i ciśnienia w rejestrze 0xF4.
 */
#define BMP280_OS_SKIPPED           0x00 // Pomiar wyłączony (Skip)
#define BMP280_OS_1X                0x01 // 16 bit / 2.62 Pa (Ultra Low Power)
#define BMP280_OS_2X                0x02 // 17 bit / 1.31 Pa (Low Power)
#define BMP280_OS_4X                0x03 // 18 bit / 0.66 Pa (Standard)
#define BMP280_OS_8X                0x04 // 19 bit / 0.33 Pa (High Resolution)
#define BMP280_OS_16X               0x05 // 20 bit / 0.16 Pa (Ultra High Resolution)

/**
 * @brief Tryby pracy (Power Modes).
 * Ustawiane w rejestrze 0xF4 (bity mode[1:0]).
 */
#define BMP280_MODE_SLEEP           0x00 // Tryb uśpienia (brak pomiarów)
#define BMP280_MODE_FORCED          0x01 // Wykonaj jeden pomiar i wróć do Sleep
#define BMP280_MODE_NORMAL          0x03 // Wykonuj pomiary cyklicznie (z przerwą Standby)

/**
 * @brief Czas Standby (t_sb).
 * Czas oczekiwania między pomiarami w trybie NORMAL.
 * Ustawiane w rejestrze 0xF5 (bity t_sb[2:0]).
 */
#define BMP280_STANDBY_0_5_MS       0x00
#define BMP280_STANDBY_62_5_MS      0x01
#define BMP280_STANDBY_125_MS       0x02
#define BMP280_STANDBY_250_MS       0x03
#define BMP280_STANDBY_500_MS       0x04
#define BMP280_STANDBY_1000_MS      0x05
#define BMP280_STANDBY_2000_MS      0x06
#define BMP280_STANDBY_4000_MS      0x07

/**
 * @brief Filtr IIR (Infinite Impulse Response).
 * Wygładza nagłe skoki ciśnienia (np. od podmuchu wiatru).
 * Ustawiane w rejestrze 0xF5 (bity filter[2:0]).
 */
#define BMP280_FILTER_OFF           0x00
#define BMP280_FILTER_2             0x01
#define BMP280_FILTER_4             0x02
#define BMP280_FILTER_8             0x03
#define BMP280_FILTER_16            0x04


typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_data_t;

typedef struct {
    i2c_port_t port;            // Numer portu I2C (np. I2C_NUM_0)
    uint8_t i2c_addr;           // Adres urządzenia (0x76 lub 0x77)
    int32_t t_fine;             // Zmienna pomocnicza do kompensacji (wymagana przez algorytm Bosch)
    bmp280_calib_data_t calib;  // Dane kalibracyjne pobrane z czujnika przy starcie
} bmp280_t;

/* ==========================================================
 * FUNKCJE (API)
 * ========================================================== */

/**
 * @brief Inicjalizacja czujnika BMP280.
 * Funkcja sprawdza ID układu, wykonuje reset programowy oraz wczytuje
 * fabryczne dane kalibracyjne niezbędne do poprawnych odczytów.
 * * @param dev Wskaźnik do struktury urządzenia.
 * @param port Port I2C (musi być wcześniej zainicjalizowany przez i2c_driver_install).
 * @param addr Adres czujnika (BMP280_I2C_ADDR_0 lub BMP280_I2C_ADDR_1).
 * @return ESP_OK w przypadku sukcesu, lub kod błędu.
 */
esp_err_t bmp280_init(bmp280_t *dev, i2c_port_t port, uint8_t addr);

/**
 * @brief Deinicjalizacja urządzenia.
 * Usypia czujnik (tryb SLEEP) i zeruje strukturę sterownika.
 */
esp_err_t bmp280_delete(bmp280_t *dev);

/**
 * @brief Odczyt temperatury i ciśnienia.
 * Funkcja pobiera surowe dane z rejestrów i przelicza je na jednostki fizyczne
 * używając algorytmu kompensacji producenta.
 * * @param dev Wskaźnik do struktury urządzenia.
 * @param temp Wskaźnik na zmienną, gdzie zapisać temperaturę [°C].
 * @param pres Wskaźnik na zmienną, gdzie zapisać ciśnienie [Pa].
 * @return ESP_OK w przypadku sukcesu.
 */
esp_err_t bmp280_read_float(bmp280_t *dev, float *temp, float *pres);


/**
 * @brief Ustawia tryb pracy czujnika (Sleep, Forced, Normal).
 * Modyfikuje rejestr CTRL_MEAS (0xF4), nie zmieniając ustawień oversamplingu.
 */
esp_err_t bmp280_set_mode(bmp280_t *dev, uint8_t mode);

/**
 * @brief Ustawia oversampling dla temperatury.
 * Modyfikuje rejestr CTRL_MEAS (0xF4), bity [7:5].
 */
esp_err_t bmp280_set_temp_oversampling(bmp280_t *dev, uint8_t os);

/**
 * @brief Ustawia oversampling dla ciśnienia.
 * Modyfikuje rejestr CTRL_MEAS (0xF4), bity [4:2].
 */
esp_err_t bmp280_set_press_oversampling(bmp280_t *dev, uint8_t os);

/**
 * @brief Ustawia współczynnik filtra IIR.
 * Modyfikuje rejestr CONFIG (0xF5), bity [4:2].
 */
esp_err_t bmp280_set_iir_filter(bmp280_t *dev, uint8_t filter);

/**
 * @brief Ustawia czas czuwania (Standby Time) w trybie NORMAL.
 * Modyfikuje rejestr CONFIG (0xF5), bity [7:5].
 */
esp_err_t bmp280_set_standby_time(bmp280_t *dev, uint8_t t_sb);

/**
 * @brief Pobiera aktualny status urządzenia.
 * Pozwala sprawdzić bit 'measuring' (czy trwa pomiar) oraz 'im_update'.
 */
esp_err_t bmp280_get_status(bmp280_t *dev, uint8_t *status);

#endif // BMP280_H
