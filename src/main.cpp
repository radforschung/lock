#include "globals.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

static const char *TAG = "main";

const unsigned TX_INTERVAL = 60;

Preferences preferences;
Lock lock;
Location location;

static osjob_t sendLockStatusJob;
static osjob_t sendLocationWifiJob;
QueueHandle_t taskQueue;
QueueHandle_t loraSendQueue = NULL;

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
  uint8_t msg[] = {0x01, ((!lock.isOpen()) ? 0x01 : 0x02)};
  loraSend(LORA_PORT_LOCK_STATUS, msg, sizeof(msg));

  // Next TX is scheduled after TX_COMPLETE event.
  ostime_t nextSendAt = os_getTime() + sec2osticks(TX_INTERVAL);
  os_setTimedCallback(&sendLockStatusJob, nextSendAt,
                      FUNC_ADDR(sendLockStatus));
  ESP_LOGI(TAG,
           "do=schedule job=sendLockStatusJob callback=sendLockStatus at=%lu",
           nextSendAt);
}

void sendWifis(osjob_t *j) {
  location.scanWifis();

  ostime_t nextSendAt = os_getTime() + sec2osticks(45);
  os_setTimedCallback(&sendLocationWifiJob, nextSendAt, FUNC_ADDR(sendWifis));
  ESP_LOGI(TAG, "do=schedule job=sendLocationWifiJob callback=sendWifis at=%lu",
           nextSendAt);
}

static char tag[] = "test_intr";
static QueueHandle_t q1;

static void handler(void *args) {
  gpio_isr_handler_remove(PIN_LOCK_LATCH_SWITCH);
  gpio_num_t gpio;
  gpio = PIN_LOCK_LATCH_SWITCH;
  xQueueSendToBackFromISR(q1, &gpio, NULL);
}

static void lockswitch_task(void *ignore) {
  ESP_LOGD(tag, ">> lockswitch_task");
  gpio_num_t gpio;
  q1 = xQueueCreate(10, sizeof(gpio_num_t));

  //TODO clean this up, mode etc is already set in lock.cpp
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
      ESP_LOGI(tag, "Lock state changed: %d", open);
      os_setCallback(&sendLockStatusJob, sendLockStatus);
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

void setup() {
  // disable brownout detection
  (*((volatile uint32_t *)ETS_UNCACHED_ADDR((DR_REG_RTCCNTL_BASE + 0xd4)))) = 0;

  Serial.begin(115200);
  delay(1000);
  preferences.begin("lock32", false);
  lock = Lock();

  taskQueue = xQueueCreate(10, sizeof(int));
  if (taskQueue == NULL) {
    ESP_LOGE(TAG, "error=\"error creating task queue\"");
  }

  setupLoRa();

  ESP_LOGI(TAG, "msg=\"hello world\" version=0.0.2");

  preferences.end();
  // Create Tasks for handling switch interrupts
  xTaskCreate(lockswitch_task,   /* Task function. */
              "lockswitch_task", /* name of task. */
              10000,             /* Stack size of task */
              NULL,              /* parameter of the task */
              1,                 /* priority of the task */
              NULL);

  os_setCallback(&sendLockStatusJob, FUNC_ADDR(sendLockStatus));

  location = Location();
  os_setCallback(&sendLocationWifiJob, FUNC_ADDR(sendWifis));
}

void loop() {
  while (1) {
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
