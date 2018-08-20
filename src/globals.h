#include <Arduino.h>
#include <Bounce2.h>
#include <Preferences.h>

//#define USE_MODEL_NODEMCU32
#define USE_MODEL_TTGOV2

#ifdef USE_MODEL_NODEMCU32
  #define PIN_LOCK_MOTOR           GPIO_NUM_14
  #define PIN_LOCK_ROTATION_SWITCH GPIO_NUM_12 // (PULLUP!)  LOW: default, HIGH: motor presses against (rotation complete)
  #define PIN_LOCK_LATCH_SWITCH    GPIO_NUM_13 // (PULLUP!) LOW: closed, HIGH: open
#endif
#ifdef USE_MODEL_TTGOV2
  #define PIN_LOCK_MOTOR           GPIO_NUM_15
  #define PIN_LOCK_ROTATION_SWITCH GPIO_NUM_2 // (PULLUP!)  LOW: default, HIGH: motor presses against (rotation complete)
  #define PIN_LOCK_LATCH_SWITCH    GPIO_NUM_4 // (PULLUP!) LOW: closed, HIGH: open

  #define PIN_SPI_SS    GPIO_NUM_18 // HPD13A NSS/SEL (Pin4) SPI Chip Select Input
  #define PIN_SPI_MOSI  GPIO_NUM_27 // HPD13A MOSI/DSI (Pin6) SPI Data Input
  #define PIN_SPI_MISO  GPIO_NUM_19 // HPD13A MISO/DSO (Pin7) SPI Data Output
  #define PIN_SPI_SCK   GPIO_NUM_5  // HPD13A SCK (Pin5) SPI Clock Input

  #define PIN_RST   LMIC_UNUSED_PIN // connected to ESP32 RST/EN
  #define PIN_DIO0  GPIO_NUM_26     // ESP32 GPIO26 wired on PCB to HPD13A
  #define PIN_DIO1  GPIO_NUM_33     // HPDIO1 on pcb, needs to be wired external to GPIO33
  #define PIN_DIO2  LMIC_UNUSED_PIN // 32 HPDIO2 on pcb, needs to be wired external to GPIO32 (not necessary for LoRa, only FSK)
#endif

#include "lock.h"
#include "lorawan.h"
