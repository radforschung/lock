#include <SPI.h>
#include <lmic.h>
#include <hal/hal.h>
#include "config.h"

void onEvent(ev_t ev);
void os_getDevKey(u1_t *buf);
void os_getArtEui(u1_t *buf);
void os_getDevEui(u1_t *buf);
void lorawan_loop(void *pvParameters);
void lorawan_init(Preferences preferences);
