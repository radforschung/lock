// fdev_setup_stream doesn't exist on esp32:
// https://github.com/espressif/arduino-esp32/issues/1123
// workaround by voiding stuff, mapping lmic_printf to esp logging functions
#ifndef _lock_lorawan_h
#define _lock_lorawan_h

#define _FDEV_SETUP_WRITE 0
#define fdev_setup_stream(...) void(##__VA_ARGS__)

#define CFG_eu868 1
#define CFG_sx1276_radio 1
#define LMIC_DEBUG_LEVEL 2

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

#include <Preferences.h>
#include <SPI.h>
#include <lmic.h>

#undef lmic_printf
#define lmic_printf(f, ...) ESP_LOGV("lora", f, ##__VA_ARGS__)

#include <hal/hal.h>
#include "config.h"

extern QueueHandle_t loraSendQueue;
extern QueueHandle_t taskQueue;

// maximum number of messages in payload send queue
#define LORA_SEND_QUEUE_SIZE 10
// maximum size of payload block per transmit
#define LORA_PAYLOAD_BUFFER_SIZE 51
// Struct holding payload for data send queue

#define LORA_PORT_LOCK_STATUS 1
#define LORA_PORT_LOCATION_GPS 10
#define LORA_PORT_LOCATION_WIFI 11

typedef struct {
  uint8_t size;
  uint8_t port;
  uint8_t payload[LORA_PAYLOAD_BUFFER_SIZE];
} MessageBuffer_t;

void onEvent(ev_t ev);
void os_getDevKey(u1_t *buf);
void os_getArtEui(u1_t *buf);
void os_getDevEui(u1_t *buf);
void lorawan_loop(void *pvParameters);
void lorawan_init(Preferences preferences);
void processSendBuffer();
bool loraSend(uint8_t port, uint8_t *msg, uint8_t size);
void setupLoRa();
void sendLockStatus(osjob_t *j);
void sendWifis(osjob_t *j);

const unsigned TX_INTERVAL2 = 60;

#endif // _lock_lorawan_h
