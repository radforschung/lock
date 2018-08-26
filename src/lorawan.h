// fdev_setup_stream doesn't exist on esp32:
// https://github.com/espressif/arduino-esp32/issues/1123
// workaround by voiding stuff, mapping lmic_printf to esp logging functions
#define _FDEV_SETUP_WRITE 0
#define fdev_setup_stream(...) void(##__VA_ARGS__)

#define CFG_eu868 1
#define CFG_sx1276_radio 1
#define LMIC_DEBUG_LEVEL 2

#include <Preferences.h>
#include <SPI.h>
#include <lmic.h>

#undef lmic_printf
#define lmic_printf(f, ...) ESP_LOGV("lora", f, ##__VA_ARGS__)

#include <hal/hal.h>
#include "config.h"

void onEvent(ev_t ev);
void os_getDevKey(u1_t *buf);
void os_getArtEui(u1_t *buf);
void os_getDevEui(u1_t *buf);
void lorawan_loop(void *pvParameters);
void lorawan_init(Preferences preferences);
