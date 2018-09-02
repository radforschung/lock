#include "globals.h"
#include "main.h"

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
  log_d(">> lockswitch_task");
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
    xQueueReceive(q1, &gpio, portMAX_DELAY);

    bool open = digitalRead(PIN_LOCK_LATCH_SWITCH);
    if (lastState != open) {
      log_i("Lock state changed: %d", open);
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

  ostime_t nextAt = os_getTime() + sec2osticks(45);
  os_setTimedCallback(&periodicTask, nextAt, FUNC_ADDR(periodicTaskSubmitter));
  log_i("do=schedule job=periodicTask callback=periodicTaskSubmitter at=%lu",
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
    log_e("error=\"error creating task queue\"");
  }

  setupLoRa();

  log_i("msg=\"hello world\" version=0.0.2");

  // Create Tasks for handling switch interrupts
  xTaskCreate(lockswitch_task,   /* Task function. */
              "lockswitch_task", /* name of task. */
              10000,             /* Stack size of task */
              NULL,              /* parameter of the task */
              1,                 /* priority of the task */
              NULL);

  xQueueSend(taskQueue, &TASK_SEND_LOCK_STATUS, portMAX_DELAY);

  location = Location();
  xQueueSend(taskQueue, &TASK_SEND_LOCATION_WIFI, portMAX_DELAY);

  // TODO: remove periodicTask. currently only there for debugging
  ostime_t nextAt = os_getTime() + sec2osticks(45);
  os_setTimedCallback(&periodicTask, nextAt, FUNC_ADDR(periodicTaskSubmitter));
}

void loop() {
  while (1) {
    int task;
    xQueueReceive(taskQueue, &task, 0);
    if (task) {
      switch (task) {
      case TASK_UNLOCK:
        log_d("task=unlock");
        lock.open();
        break;
      case TASK_RESTART:
        log_d("task=restart");
        esp_restart();
        break;
      case TASK_SEND_LOCK_STATUS:
        log_d("task=\"send lock status\"");
        sendLockStatus();
        break;
      case TASK_SEND_LOCATION_WIFI:
        log_d("task=\"send location wifi\"");
        sendWifis();
        break;
      default:
        log_w("error=\"unknown task submitted\"");
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
