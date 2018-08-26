#include "globals.h"

static const char *TAG = "main";

const unsigned TX_INTERVAL = 60;

Preferences preferences;
Lock lock;
Location location;

static osjob_t sendjob;
QueueHandle_t taskQueue;

void setupLoRa() {
  lorawan_init(preferences);
  ESP_LOGI(TAG, "start=loratask");
  xTaskCreatePinnedToCore(lorawan_loop, "loraloop", 2048, (void *)1,
                          (5 | portPRIVILEGE_BIT), NULL, 1);
}

void do_send(osjob_t *j) {
  uint8_t message[] = {0x01, 0x00, 0x00};
  if (!lock.isOpen()) {
    message[1] = 0x01;
  } else {
    message[1] = 0x02;
  }

  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    ESP_LOGD(LORA_TAG, "msg=\"OP_TXRXPEND, not sending\" loraseq=%d",
             LMIC.seqnoUp);
  } else {
    Preferences pref;
    pref.begin("lock32", false);
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, message, sizeof(message) - 1, 0);
    pref.putUInt("counter", LMIC.seqnoUp);
    ESP_LOGD(LORA_TAG, "msg=\"sending packet\" loraseq=%d", LMIC.seqnoUp);

    pref.end();
  }

  // Next TX is scheduled after TX_COMPLETE event.
  os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL),
                      do_send);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  preferences.begin("lock32", false);
  lock = Lock();
  location = Location();
  taskQueue = xQueueCreate(10, sizeof(int));
  if (taskQueue == NULL) {
    ESP_LOGW(TAG, "msg=\"error creating task queue\"");
  }

  setupLoRa();

  ESP_LOGI(TAG, "msg=\"hello world\" version=0.0.1");

  preferences.end();
  os_setCallback(&sendjob, do_send);
}

boolean lastState = false;

void loop() {
  while (1) {
    bool open = lock.isOpen();
    // report change
    if (lastState != open) {
      ESP_LOGD(TAG, "change=true");
      os_setCallback(&sendjob, do_send);
      lastState = open;
    }

    if (!open) {
      ESP_LOGD(TAG, "lock=closed");
      // lock.open();
    } else {
      if (!lock.motorIsParked()) {
        lock.open();
      }
      ESP_LOGD(TAG, "lock=open");
    }

    int task;
    xQueueReceive(taskQueue, &task, 0);
    if (task) {
      if (task == TASK_UNLOCK) {
        ESP_LOGD(TAG, "task=unlock");
        lock.open();
        task = 0;
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
