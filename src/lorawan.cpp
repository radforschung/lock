#include "globals.h"
#include "lock.h"

static const char *TAG = "lora";

QueueHandle_t loraSendQueue = NULL;
QueueHandle_t loraParseQueue = NULL;

const lmic_pinmap lmic_pins = {.nss = PIN_SPI_LORA_SS,
                               .rxtx = LMIC_UNUSED_PIN,
                               .rst = PIN_RST,
                               .dio = {PIN_DIO0, PIN_DIO1, PIN_DIO2}};

// they have to exist but do nothing:
void os_getArtEui(u1_t *buf) {}
void os_getDevEui(u1_t *buf) {}
void os_getDevKey(u1_t *buf) {}

void lorawan_init(uint8_t sequenceNum) {
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

  LMIC.seqnoUp = sequenceNum;
  ESP_LOGD(TAG, "loraseq=%d", LMIC.seqnoUp);
}

// LMIC FreeRTos Task
void lorawan_loop(void *pvParameters) {
  ESP_LOGD(TAG, "task=lorawan_loop state=enter");
  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  Preferences pref;
  pref.begin(PREFERENCES_KEY, false);
  uint8_t sequenceNum = pref.getUInt("counter", 1);
  lorawan_init(sequenceNum);
  pref.end();

  ESP_LOGI(TAG, "task=lorawan_loop state=active");
  while (1) {
    os_runloop_once();
    processSendBuffer();
    processLoraParse();
    // reset watchdog # CONFIG_FREERTOS_HZ=1000
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
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

      uint8_t port = 1;
      if (LMIC.txrxFlags & TXRX_PORT) {
        port = LMIC.frame[LMIC.dataBeg - 1];
      }

      MessageBuffer_t recvBuffer;
      recvBuffer.size = LMIC.dataLen;
      recvBuffer.port = port;
      memcpy(recvBuffer.payload, LMIC.frame + LMIC.dataBeg, LMIC.dataLen);

      xQueueSendToBack(loraParseQueue, (void *)&recvBuffer, (TickType_t)0);
    }
  }
}

void processLoraParse() {
  if (loraParseQueue == NULL) {
    return;
  }
  MessageBuffer_t recvBuffer;
  if (xQueueReceive(loraParseQueue, &recvBuffer, (TickType_t)0) == pdTRUE) {
    if (recvBuffer.size >= 2 && recvBuffer.payload[1] == 0x01) {
      xQueueSend(taskQueue, &TASK_UNLOCK, portMAX_DELAY);
      return;
    }
    if (recvBuffer.size >= 2 && recvBuffer.payload[1] == 0x02) {
      xQueueSend(taskQueue, &TASK_RESTART, portMAX_DELAY);
      return;
    }
  }
}

bool loraSend(uint8_t port, uint8_t *msg, uint8_t size) {
  if (loraSendQueue == NULL) {
    ESP_LOGE(TAG, "error=\"called loraSend before queue is initialized\"");
    return false;
  }

  MessageBuffer_t sendBuffer;
  sendBuffer.size = size;
  sendBuffer.port = port;
  memcpy(sendBuffer.payload, msg, size);

  if (xQueueSendToBack(loraSendQueue, (void *)&sendBuffer, (TickType_t)0) ==
      pdTRUE) {
    ESP_LOGI(TAG, "queue=loraSend action=add port=%d bytes=%d", sendBuffer.port,
             sendBuffer.size);
    return true;
  }
  return false;
}

void processSendBuffer() {
  // skip if LoRa is busy and don't get data from the queue
  if ((LMIC.opmode & (OP_JOINING | OP_REJOIN | OP_TXDATA | OP_POLL)) != 0) {
    return;
  }

  if (loraSendQueue == NULL) {
    return;
  }

  MessageBuffer_t sendBuffer;
  if (xQueueReceive(loraSendQueue, &sendBuffer, (TickType_t)0) == pdTRUE) {
    // sendBuffer now contains the MessageBuffer
    // with the data to send from the queue
    Preferences pref;
    pref.begin(PREFERENCES_KEY, false);
    LMIC_setTxData2(sendBuffer.port, sendBuffer.payload, sendBuffer.size, 0);
    pref.putUInt("counter", LMIC.seqnoUp);
    ESP_LOGD(LORA_TAG, "msg=\"sending packet\" port=%d loraseq=%d bytes=%d",
             sendBuffer.port, LMIC.seqnoUp, sendBuffer.size);
    pref.end();
  }
}
