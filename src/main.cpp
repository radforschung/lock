#include "globals.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

static const char *TAG = "main";

const unsigned TX_INTERVAL = 60;

Preferences preferences;
Lock lock;

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

static char tag[] = "test_intr";
static QueueHandle_t q1;

static void handler(void *args) {
	gpio_num_t gpio;
	gpio = PIN_LOCK_LATCH_SWITCH;
	xQueueSendToBackFromISR(q1, &gpio, NULL);
}

static void test1_task(void *ignore) {
	ESP_LOGD(tag, ">> test1_task");
	gpio_num_t gpio;
	q1 = xQueueCreate(10, sizeof(gpio_num_t));

	gpio_config_t gpioConfig;
	gpioConfig.pin_bit_mask = GPIO_SEL_4;
	gpioConfig.mode         = GPIO_MODE_INPUT;
	gpioConfig.pull_up_en   = GPIO_PULLUP_ENABLE;
	gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpioConfig.intr_type    = GPIO_INTR_ANYEDGE;
	gpio_config(&gpioConfig);

	gpio_install_isr_service(0);
	gpio_isr_handler_add(PIN_LOCK_LATCH_SWITCH, handler, NULL	);
	while(1) {
		ESP_LOGD(tag, "Waiting on interrupt queue");
		BaseType_t rc = xQueueReceive(q1, &gpio, portMAX_DELAY);
		ESP_LOGD(tag, "Woke from interrupt queue wait: %d", rc);
	}
	vTaskDelete(NULL);
}


void setup() {
  Serial.begin(115200);
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
  xTaskCreate(
      test1_task,           /* Task function. */
      "test1_task",        /* name of task. */
      10000,                    /* Stack size of task */
      NULL,                     /* parameter of the task */
      1,                        /* priority of the task */
      NULL);
  os_setCallback(&sendjob, sendLockStatus);
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
