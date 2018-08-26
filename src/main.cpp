#include "globals.h"

static const char *TAG = "main";

const unsigned TX_INTERVAL = 60;

Preferences preferences;
Lock lock;
Location location;

static osjob_t sendjob;
QueueHandle_t taskQueue;
QueueHandle_t loraSendQueue;

void setupLoRa() {
  loraSendQueue = xQueueCreate(LORA_SEND_QUEUE_SIZE, sizeof(MessageBuffer_t));
  if (loraSendQueue == 0) {
    ESP_LOGE(TAG, "error=\"creation of lora send queue failed\"");
  }
  lorawan_init(preferences);
  ESP_LOGI(TAG, "start=loratask");
  xTaskCreatePinnedToCore(lorawan_loop, "loraloop", 2048, (void *)1,
                          (5 | portPRIVILEGE_BIT), NULL, 1);
}

void sendLockStatus(osjob_t *j) {
  uint8_t msg[] = {0x01, ((!lock.isOpen()) ? 0x01 : 0x02), 0x00};
  loraSend(msg);

  // Next TX is scheduled after TX_COMPLETE event.
  os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL),
                      sendLockStatus);
}

void setup() {
  Serial.begin(115200);
  //Magic TTGO Lora fix
  (*((volatile uint32_t *)ETS_UNCACHED_ADDR((DR_REG_RTCCNTL_BASE + 0xd4)))) = 0;
  delay(1000);
  preferences.begin("lock32", false);
  lock = Lock();
  
  taskQueue = xQueueCreate(10, sizeof(int));
  if (taskQueue == NULL) {
    ESP_LOGW(TAG, "msg=\"error creating task queue\"");
  }

  setupLoRa();

  ESP_LOGI(TAG, "msg=\"hello world\" version=0.0.1");
  
  preferences.end();
  os_setCallback(&sendjob, sendLockStatus);

  location = Location();
  location.scanWifis();
}

boolean lastState = false;

void loop() {
  while (1) {
    bool open = lock.isOpen();
    // report change
    if (lastState != open) {
      ESP_LOGD(TAG, "change=true");
      os_setCallback(&sendjob, sendLockStatus);
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

    processSendBuffer();

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
