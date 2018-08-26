#include "globals.h"

static const char *TAG = "lora";

const lmic_pinmap lmic_pins = {.nss = PIN_SPI_SS,
                               .rxtx = LMIC_UNUSED_PIN,
                               .rst = PIN_RST,
                               .dio = {PIN_DIO0, PIN_DIO1, PIN_DIO2}};

// they have to exist but do nothing:
void os_getArtEui(u1_t *buf) {}
void os_getDevEui(u1_t *buf) {}
void os_getDevKey(u1_t *buf) {}

void lorawan_init(Preferences preferences) {
  // init spi before
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, 0x00);

  // initialize LoRaWAN LMIC run-time environment
  os_init();

  // reset LMIC MAC state
  LMIC_reset();

  // This tells LMIC to make the receive windows bigger, in case your clock is
  // 1% faster or slower.
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

  // Set static session parameters.
  // copy for esp
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession(0x1, DEVADDR, nwkskey, appskey);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink
  // (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF9, 15);

  LMIC.seqnoUp = preferences.getUInt("counter", 1);
  ESP_LOGD(TAG, "loraseq=%d", LMIC.seqnoUp);
}

// LMIC FreeRTos Task
void lorawan_loop(void *pvParameters) {
  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check
  while (1) {
    os_runloop_once();
    // reset watchdog # CONFIG_FREERTOS_HZ=1000
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void onEvent(ev_t ev) {
  if (ev == EV_JOINED) {
    LMIC_setAdrMode(1);
    LMIC_setLinkCheckMode(1);
    LMIC_setDrTxpow(DR_SF9, 15);
  }
  if (ev == EV_RXCOMPLETE) {
    // data received in ping slot
    ESP_LOGD(TAG, "loraEvent=EV_RXCOMPLETE");
    return;
  }
  if (ev == EV_TXCOMPLETE) {
    ESP_LOGD(TAG, "loraEvent=EV_TXCOMPLETE");
    if (LMIC.txrxFlags & TXRX_ACK) {
      ESP_LOGD(TAG, "msg=\"Received ack\"");
    }
    if (LMIC.dataLen) {
      ESP_LOGD(TAG, "payloadLength=%d", LMIC.dataLen);
      Serial.print(F(" data=0x"));
      for (int i = 0; i < LMIC.dataLen; i++) {
        if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
          Serial.print(F("0"));
        }
        Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
      }
      Serial.println();

      // FIXME: throw data into a parsing event
      if (LMIC.dataLen >= 2 && LMIC.frame[LMIC.dataBeg + 1] == 0x01) {
        xQueueSend(taskQueue, &TASK_UNLOCK, portMAX_DELAY);
      }
    }
  }
}
