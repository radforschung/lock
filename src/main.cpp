#include "globals.h"
#include "main.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

QueueHandle_t taskQueue;

static osjob_t periodicTask;

void periodicTaskSubmitter(osjob_t *j) {
  xQueueSend(taskQueue, &TASK_SEND_LOCK_STATUS, portMAX_DELAY);
  xQueueSend(taskQueue, &TASK_SEND_LOCATION_WIFI, portMAX_DELAY);
  xQueueSend(taskQueue, &TASK_SEND_LOCATION_GPS, portMAX_DELAY);

  ostime_t nextAt = os_getTime() + sec2osticks(60 * 5);
  os_setTimedCallback(&periodicTask, nextAt, FUNC_ADDR(periodicTaskSubmitter));
  ESP_LOGI(TAG,
           "do=schedule job=periodicTask callback=periodicTaskSubmitter at=%lu",
           nextAt);
}

void setup() {
  // disable brownout detection
  (*((volatile uint32_t *)ETS_UNCACHED_ADDR((DR_REG_RTCCNTL_BASE + 0xd4)))) = 0;

  Serial.begin(115200);
  delay(1000);

  taskQueue = xQueueCreate(10, sizeof(int));
  if (taskQueue == NULL) {
    ESP_LOGE(TAG, "error=\"error creating task queue\"");
  }

  lockQueue = xQueueCreate(10, sizeof(int));
  if (lockQueue == NULL) {
    ESP_LOGE(TAG, "error=\"error creating lock queue\"");
  }

  loraSendQueue = xQueueCreate(LORA_SEND_QUEUE_SIZE, sizeof(MessageBuffer_t));
  if (loraSendQueue == NULL) {
    ESP_LOGE(TAG, "error=\"creation of lora send queue failed\"");
  }

  loraParseQueue = xQueueCreate(LORA_PARSE_QUEUE_SIZE, sizeof(MessageBuffer_t));
  if (loraParseQueue == NULL) {
    ESP_LOGE(TAG, "error=\"creation of lora parse queue failed\"");
  }

  gpsQueue = xQueueCreate(3, 1);
  if (gpsQueue == NULL) {
    ESP_LOGE(TAG, "error=\"error creating gps queue\"");
  }

  wifiQueue = xQueueCreate(3, 1);
  if (wifiQueue == NULL) {
    ESP_LOGE(TAG, "error=\"error creating wifi queue\"");
  }

  epaperQueue = xQueueCreate(3, sizeof(int));
  if (epaperQueue == NULL) {
    ESP_LOGE(TAG, "error=\"error creating epaper queue\"");
  }

  ESP_LOGI(TAG, "msg=\"hello world\" version=0.0.2");

  // init spi before
  pinMode(PIN_SPI_LORA_SS, OUTPUT);
  digitalWrite(PIN_SPI_LORA_SS, HIGH);

  pinMode(PIN_SPI_EPAPER_SS, OUTPUT);
  digitalWrite(PIN_SPI_EPAPER_SS, HIGH);

  pinMode(PIN_SPI_MOSI, OUTPUT);
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, 0x00);

  xTaskCreatePinnedToCore(lorawan_loop,            // Task function.
                          "loraloop",              // name of task.
                          2048,                    // Stack size of task
                          (void *)1,               // parameter of the task
                          (5 | portPRIVILEGE_BIT), // priority of the task
                          NULL,                    // constpvCreatedTask
                          1);                      // xCoreID

  xTaskCreate(lock_task,   // Task function.
              "lock_task", // name of task.
              10000,       // Stack size of task
              NULL,        // parameter of the task
              2,           // priority of the task
              NULL);
  xTaskCreate(button_task,   // Task function.
              "button_task", // name of task.
              10000,         // Stack size of task
              NULL,          // parameter of the task
              2,             // priority of the task
              NULL);
  xTaskCreate(wifi_task,   // Task function.
              "wifi_task", // name of task.
              10000,       // Stack size of task
              NULL,        // parameter of the task
              1,           // priority of the task
              NULL);
  xTaskCreate(gps_task,   // Task function.
              "gps_task", // name of task.
              10000,      // Stack size of task
              NULL,       // parameter of the task
              1,          // priority of the task
              NULL);
  xTaskCreate(epaper_task,   // Task function.
              "epaper_task", // name of task.
              10000,         // Stack size of task
              NULL,          // parameter of the task
              1,             // priority of the task
              NULL);

  xQueueSend(taskQueue, &TASK_SEND_LOCK_STATUS, portMAX_DELAY);
  xQueueSend(taskQueue, &TASK_SEND_LOCATION_WIFI, portMAX_DELAY);
  xQueueSend(taskQueue, &TASK_SEND_LOCATION_GPS, portMAX_DELAY);
  xQueueSend(epaperQueue, &EPAPER_SCREEN_LOGO, portMAX_DELAY);

  // TODO: remove periodicTask. currently only there for debugging
  ostime_t nextAt = os_getTime() + sec2osticks(5 * 60);
  os_setTimedCallback(&periodicTask, nextAt, FUNC_ADDR(periodicTaskSubmitter));
}

void loop() {
  void *nothing;
  while (1) {
    int task;
    if (xQueueReceive(taskQueue, &task, 0) == pdTRUE) {
      switch (task) {
      case TASK_UNLOCK:
        ESP_LOGD(TAG, "task=unlock");
        xQueueSend(lockQueue, &LOCK_TASK_UNLOCK, portMAX_DELAY);
        break;
      case TASK_RESTART:
        ESP_LOGD(TAG, "task=restart");
        esp_restart();
        break;
      case TASK_SEND_LOCK_STATUS:
        ESP_LOGD(TAG, "task=\"send lock status\"");
        xQueueSend(lockQueue, &LOCK_TASK_SEND_STATUS, portMAX_DELAY);
        break;
      case TASK_SEND_LOCATION_WIFI:
        ESP_LOGD(TAG, "task=\"send location wifi\"");
        xQueueSend(wifiQueue, &nothing, portMAX_DELAY);
        break;
      case TASK_SEND_LOCATION_GPS:
        ESP_LOGD(TAG, "task=\"send location gps\"");
        xQueueSend(gpsQueue, &nothing, portMAX_DELAY);
        break;
      default:
        ESP_LOGW(TAG, "error=\"unknown task submitted\"");
        break;
      }
      task = 0;
    }

    processSerial();

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
