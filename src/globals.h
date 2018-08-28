#ifndef _lock_globals_h
#define _lock_globals_h
#include <Arduino.h>

// overriding with our own, https://brandur.org/logfmt compatible log format:
#undef ARDUHAL_LOG_FORMAT
#define ARDUHAL_LOG_FORMAT(letter, format)                                     \
  ARDUHAL_LOG_COLOR_##letter                                                   \
      "time=%d level=" #letter                                                 \
      " file=%s line=%u method=%s " format ARDUHAL_LOG_RESET_COLOR "\r\n",     \
      hal_ticks(), pathToFileName(__FILE__), __LINE__, __FUNCTION__

#include <Bounce2.h>
#include <Preferences.h>

#define PIN_LOCK_MOTOR GPIO_NUM_15
// rotation switch: (PULLUP!) LOW: default, HIGH: motor presses against
// (rotation complete)
#define PIN_LOCK_ROTATION_SWITCH GPIO_NUM_2
// latch switch: (PULLUP!) LOW: closed, HIGH: open
#define PIN_LOCK_LATCH_SWITCH GPIO_NUM_4

#define PIN_SPI_SS GPIO_NUM_18   // HPD13A NSS/SEL (Pin4) SPI Chip Select Input
#define PIN_SPI_MOSI GPIO_NUM_27 // HPD13A MOSI/DSI (Pin6) SPI Data Input
#define PIN_SPI_MISO GPIO_NUM_19 // HPD13A MISO/DSO (Pin7) SPI Data Output
#define PIN_SPI_SCK GPIO_NUM_5   // HPD13A SCK (Pin5) SPI Clock Input

#define PIN_RST LMIC_UNUSED_PIN // connected to ESP32 RST/EN
#define PIN_DIO0 GPIO_NUM_26    // ESP32 GPIO26 wired on PCB to HPD13A
// HPDIO1 on pcb, needs to be wired external to GPIO33
#define PIN_DIO1 GPIO_NUM_33
// 32 HPDIO2 on pcb, needs to be wired external to GPIO32
// (not necessary for LoRa, only FSK)
#define PIN_DIO2 LMIC_UNUSED_PIN

#include "lock.h"
#include "lorawan.h"
#include "location.h"

extern QueueHandle_t taskQueue;
static const int TASK_UNLOCK = 1;

#endif // _lock_globals_h
