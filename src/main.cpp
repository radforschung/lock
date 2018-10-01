#include "globals.h"
#include "main.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

QueueHandle_t taskQueue;

static osjob_t periodicTask;
static QueueHandle_t q1;

static void handler(void *args) {
  gpio_isr_handler_remove(PIN_LOCK_LATCH_SWITCH);
  gpio_num_t gpio;
  gpio = PIN_LOCK_LATCH_SWITCH;
  xQueueSendToBackFromISR(q1, &gpio, NULL);
}

static void lockswitch_task(void *ignore) {
  ESP_LOGD(TAG, ">> lockswitch_task");
  gpio_num_t gpio;
  q1 = xQueueCreate(10, sizeof(gpio_num_t));

  // TODO clean this up, mode etc is already set in lock.cpp
  //     only thing to be done is to set INTR_ANYEDGE
  gpio_config_t gpioConfig;
  gpioConfig.pin_bit_mask = GPIO_SEL_4;
  gpioConfig.mode = GPIO_MODE_INPUT;
  gpioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
  gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpioConfig.intr_type = GPIO_INTR_ANYEDGE;
  gpio_config(&gpioConfig);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(PIN_LOCK_LATCH_SWITCH, handler, NULL);

  boolean lastState = false;

  while (1) {
    BaseType_t rc = xQueueReceive(q1, &gpio, portMAX_DELAY);
    bool open = digitalRead(PIN_LOCK_LATCH_SWITCH);
    if (lastState != open) {
      ESP_LOGI(TAG, "Lock state changed: %d", open);
      xQueueSend(taskQueue, &TASK_SEND_LOCK_STATUS, portMAX_DELAY);
      xQueueSend(taskQueue, &TASK_SEND_LOCATION_WIFI, portMAX_DELAY);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    lastState = open;

    // TODO This might be better suited within another task,
    // the scheduler sometimes complains about exceeding watchdog limits
    if (open && !lock.motorIsParked()) {
      lock.open();
    }

    gpio_isr_handler_add(PIN_LOCK_LATCH_SWITCH, handler, NULL);
  }
  vTaskDelete(NULL);
}

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
  lock = Lock();

  taskQueue = xQueueCreate(10, sizeof(int));
  if (taskQueue == NULL) {
    ESP_LOGE(TAG, "error=\"error creating task queue\"");
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

  setupLoRa();

  // Create Task for handling switch interrupts
  xTaskCreate(lockswitch_task,   // Task function.
              "lockswitch_task", // name of task.
              10000,             // Stack size of task
              NULL,              // parameter of the task
              4,                 // priority of the task
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
    xQueueReceive(taskQueue, &task, 0);
    if (task) {
      switch (task) {
      case TASK_UNLOCK:
        ESP_LOGD(TAG, "task=unlock");
        lock.open();
        break;
      case TASK_RESTART:
        ESP_LOGD(TAG, "task=restart");
        esp_restart();
        break;
      case TASK_SEND_LOCK_STATUS:
        ESP_LOGD(TAG, "task=\"send lock status\"");
        sendLockStatus();
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

    processSendBuffer();
    processLoraParse();
    processSerial();

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
